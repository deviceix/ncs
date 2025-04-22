#include <gtest/gtest.h>
#include <ncs/containers/column.hpp>
#include <string>
#include <memory>
#include <utility>

class ColumnTest : public ::testing::Test
{
protected:
	ncs::Column column;
};

class TestClass
{
public:
	static int construct_count;
	static int copy_count;
	static int move_count;
	static int destruct_count;

	int value;
	std::string name;

	TestClass() : value(0), name("default")
	{
		++construct_count;
	}

	TestClass(const int v, std::string n) : value(v), name(std::move(n))
	{
		++construct_count;
	}

	TestClass(const TestClass &other) : value(other.value), name(other.name)
	{
		++copy_count;
	}

	TestClass(TestClass &&other) noexcept : value(other.value), name(std::move(other.name))
	{
		++move_count;
	}

	~TestClass()
	{
		++destruct_count;
	}

	static void resetCounters()
	{
		construct_count = 0;
		copy_count = 0;
		move_count = 0;
		destruct_count = 0;
	}
};

int TestClass::construct_count = 0;
int TestClass::copy_count = 0;
int TestClass::move_count = 0;
int TestClass::destruct_count = 0;

TEST_F(ColumnTest, Initialization)
{
	EXPECT_EQ(column.capacity(), 0);
	EXPECT_EQ(column.size(), 0);
	EXPECT_FALSE(column.has_dtor());
	EXPECT_FALSE(column.has_copier());
}

TEST_F(ColumnTest, LoadTrivialType)
{
	column.load<int>();
	EXPECT_EQ(column.size(), sizeof(int));
	EXPECT_FALSE(column.has_dtor());
	EXPECT_FALSE(column.has_copier());
}

TEST_F(ColumnTest, LoadNonTrivialType)
{
	column.load<std::string>();
	EXPECT_EQ(column.size(), sizeof(std::string));
	EXPECT_TRUE(column.has_dtor());
	EXPECT_TRUE(column.has_copier());
}

TEST_F(ColumnTest, ConstructTrivialType)
{
	column.load<int>();
	auto row = column.construct_at<int>(0, 42);
	EXPECT_EQ(column.capacity(), 1);
	const auto value = column.get_as<int>(row);
	ASSERT_TRUE(value);
	EXPECT_EQ(*value, 42);
}

TEST_F(ColumnTest, ConstructNonTrivialType)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	TestClass obj(42, "Test");
	auto row = column.construct_at<TestClass>(0, obj);

	EXPECT_EQ(column.capacity(), 1);
	auto value = column.get_as<TestClass>(row);
	ASSERT_NE(value, nullptr);
	EXPECT_EQ(value->value, 42);
	EXPECT_EQ(value->name, "Test");

	EXPECT_EQ(TestClass::construct_count, 1);
	EXPECT_GE(TestClass::copy_count, 1);
}

TEST_F(ColumnTest, Resize)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	TestClass obj1(1, "One");
	auto row1 = column.construct_at<TestClass>(0, obj1);
	EXPECT_EQ(column.capacity(), 1);

	TestClass obj2(2, "Two");
	auto row2 = column.construct_at<TestClass>(1, obj2);
	EXPECT_EQ(column.capacity(), 2);

	auto val1 = column.get_as<TestClass>(row1);
	auto val2 = column.get_as<TestClass>(row2);

	ASSERT_NE(val1, nullptr);
	ASSERT_NE(val2, nullptr);
	EXPECT_EQ(val1->value, 1);
	EXPECT_EQ(val1->name, "One");
	EXPECT_EQ(val2->value, 2);
	EXPECT_EQ(val2->name, "Two");
}

TEST_F(ColumnTest, LargeResize)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	std::vector<std::size_t> rows;

	for (int i = 0; i < 10; ++i)
	{
		TestClass obj(i, "Item" + std::to_string(i));
		rows.push_back(column.construct_at<TestClass>(i, obj));
	}

	EXPECT_GE(column.capacity(), 10);
	for (int i = 0; i < 10; ++i)
	{
		auto val = column.get_as<TestClass>(rows[i]);
		ASSERT_NE(val, nullptr);
		EXPECT_EQ(val->value, i);
		EXPECT_EQ(val->name, "Item" + std::to_string(i));
	}
}

TEST_F(ColumnTest, DestroyAt)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	TestClass obj(42, "Test");
	auto row = column.construct_at<TestClass>(0, obj);

	int destruct_count_before = TestClass::destruct_count;
	column.destroy_at(row);
	EXPECT_EQ(TestClass::destruct_count, destruct_count_before + 1);

	EXPECT_FALSE(column.is_constructed(row));
	EXPECT_EQ(column.get_as<TestClass>(row), nullptr);
}

TEST_F(ColumnTest, CopyConstructor)
{
	column.load<int>();
	auto row1 = column.construct_at<int>(0, 42);
	auto row2 = column.construct_at<int>(1, 43);

	ncs::Column copy(column);

	EXPECT_EQ(copy.capacity(), column.capacity());
	EXPECT_EQ(copy.size(), column.size());

	auto val1 = copy.get_as<int>(row1);
	auto val2 = copy.get_as<int>(row2);

	ASSERT_TRUE(val1 != nullptr);
	ASSERT_TRUE(val2 != nullptr);
	EXPECT_EQ(*val1, 42);
	EXPECT_EQ(*val2, 43);
}

TEST_F(ColumnTest, MoveConstructor)
{
	column.load<int>();
	auto row1 = column.construct_at<int>(0, 42);
	auto row2 = column.construct_at<int>(1, 43);

	ncs::Column moved(std::move(column));

	EXPECT_EQ(column.capacity(), 0);
	EXPECT_EQ(moved.capacity(), 2);

	auto val1 = moved.get_as<int>(row1);
	auto val2 = moved.get_as<int>(row2);

	ASSERT_TRUE(val1 != nullptr);
	ASSERT_TRUE(val2 != nullptr);
	EXPECT_EQ(*val1, 42);
	EXPECT_EQ(*val2, 43);
}

TEST_F(ColumnTest, MoveNonTrivialTypes)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	TestClass obj1(1, "One");
	TestClass obj2(2, "Two");
	auto row1 = column.construct_at<TestClass>(0, obj1);
	auto row2 = column.construct_at<TestClass>(1, obj2);

	int destruct_count_before = TestClass::destruct_count;

	ncs::Column moved(std::move(column));

	EXPECT_EQ(TestClass::destruct_count, destruct_count_before);

	auto val1 = moved.get_as<TestClass>(row1);
	auto val2 = moved.get_as<TestClass>(row2);

	ASSERT_NE(val1, nullptr);
	ASSERT_NE(val2, nullptr);
	EXPECT_EQ(val1->value, 1);
	EXPECT_EQ(val1->name, "One");
	EXPECT_EQ(val2->value, 2);
	EXPECT_EQ(val2->name, "Two");
}

TEST_F(ColumnTest, Clear)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	TestClass obj1(1, "One");
	TestClass obj2(2, "Two");
	column.construct_at<TestClass>(0, obj1);
	column.construct_at<TestClass>(1, obj2);

	const int destruct_count_before = TestClass::destruct_count;
	column.clear();

	EXPECT_EQ(TestClass::destruct_count, destruct_count_before + 2);
	EXPECT_EQ(column.capacity(), 0);
}

TEST_F(ColumnTest, MemorySafetyDuringResize)
{
	TestClass::resetCounters();
	column.load<TestClass>();

	TestClass obj1(1, "One");
	const auto row1 = column.construct_at<TestClass>(0, obj1);
	const auto ptr1_before = column.get_as<TestClass>(row1);

	std::string name_copy = ptr1_before->name;

	TestClass obj2(2, "Two");
	auto row2 = column.construct_at<TestClass>(1, obj2);

	auto ptr1_after = column.get_as<TestClass>(row1);
	[[maybe_unused]] auto ptr2 = column.get_as<TestClass>(row2);

	ASSERT_NE(ptr1_after, nullptr);
	EXPECT_EQ(ptr1_after->value, 1);
	EXPECT_EQ(ptr1_after->name, name_copy);

	EXPECT_NE(ptr1_before, ptr1_after);
}

TEST_F(ColumnTest, SwitchingTypes)
{
	column.load<std::string>();
	const auto row1 = column.construct_at<std::string>(0, "Hello");
	const auto row2 = column.construct_at<std::string>(1, "World");

	const auto str1 = column.get_as<std::string>(row1);
	const auto str2 = column.get_as<std::string>(row2);

	ASSERT_NE(str1, nullptr);
	ASSERT_NE(str2, nullptr);
	EXPECT_EQ(*str1, "Hello");
	EXPECT_EQ(*str2, "World");

	column.load<int>();
	auto row3 = column.construct_at<int>(0, 42);
	auto row4 = column.construct_at<int>(1, 43);

	auto int1 = column.get_as<int>(row3);
	auto int2 = column.get_as<int>(row4);

	ASSERT_TRUE(int1 != nullptr);
	ASSERT_TRUE(int2 != nullptr);
	EXPECT_EQ(*int1, 42);
	EXPECT_EQ(*int2, 43);
}
