#include <gtest/gtest.h>
#include <ncs/world.hpp>

struct Position
{
	float x, y, z;
	Position(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

	bool operator==(const Position &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct Velocity
{
	float x, y, z;
	Velocity(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

	bool operator==(const Velocity &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct Health
{
	int value;
	Health(int value = 0) : value(value) {}

	bool operator==(const Health &other) const
	{
		return value == other.value;
	}
};

struct Name
{
	std::string name;

	Name() = default;

	Name(std::string s) : name(std::move(s)) {}
};

class CRUDTest : public testing::Test
{
protected:
	ncs::World world;
	ncs::Entity entity = {};

	void SetUp() override
	{
		entity = world.entity();
	}
};

TEST_F(CRUDTest, ComponentSet)
{
	const Position pos = { 1.0f, 2.0f, 3.0f };
	world.set<Position>(entity, pos);

	EXPECT_TRUE(world.has<Position>(entity));

	const Position new_pos = { 4.0f, 5.0f, 6.0f };
	world.set<Position>(entity, new_pos);

	auto *pos_ptr = world.get<Position>(entity);
	ASSERT_NE(pos_ptr, nullptr);
	EXPECT_EQ(*pos_ptr, new_pos);

	const Velocity vel = { 10.0f, 20.0f, 30.0f };
	world.set<Velocity>(entity, vel);

	EXPECT_TRUE(world.has<Position>(entity));
	EXPECT_TRUE(world.has<Velocity>(entity));

	auto *vel_ptr = world.get<Velocity>(entity);
	ASSERT_NE(vel_ptr, nullptr);
	EXPECT_EQ(*vel_ptr, vel);
}

TEST_F(CRUDTest, ComponentGet)
{
	const Position pos = { 1.0f, 2.0f, 3.0f };
	const Velocity vel = { 10.0f, 20.0f, 30.0f };
	const Health health = { 100 };

	world.set<Position>(entity, pos);
	world.set<Velocity>(entity, vel);
	world.set<Health>(entity, health);

	auto *pos_ptr = world.get<Position>(entity);
	const auto *vel_ptr = world.get<Velocity>(entity);
	const auto *health_ptr = world.get<Health>(entity);

	ASSERT_TRUE(pos_ptr != nullptr);
	ASSERT_TRUE(vel_ptr != nullptr);
	ASSERT_TRUE(health_ptr != nullptr);

	EXPECT_EQ(*pos_ptr, pos);
	EXPECT_EQ(*vel_ptr, vel);
	EXPECT_EQ(*health_ptr, health);

	struct NonExistent {};
	auto *none_ptr = world.get<NonExistent>(entity);
	EXPECT_EQ(none_ptr, nullptr);

	pos_ptr->x = 99.0f;

	auto *updated_pos = world.get<Position>(entity);
	ASSERT_NE(updated_pos, nullptr);
	EXPECT_EQ(updated_pos->x, 99.0f);
}

TEST_F(CRUDTest, ComponentHas)
{
	world.set<Position>(entity, { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(entity, { 10.0f, 20.0f, 30.0f });

	EXPECT_TRUE(world.has<Position>(entity));
	EXPECT_TRUE(world.has<Velocity>(entity));

	EXPECT_FALSE(world.has<Health>(entity));

	world.set<Health>(entity, { 100 });
	EXPECT_TRUE(world.has<Health>(entity));

	const ncs::Entity invalid_entity = ncs::World::encode_entity(999999, 0);
	EXPECT_FALSE(world.has<Position>(invalid_entity));

	const ncs::Entity entity2 = world.entity();
	world.set<Health>(entity2, Health { 50 });

	EXPECT_TRUE(world.has<Health>(entity2));
	EXPECT_FALSE(world.has<Position>(entity2));
	EXPECT_FALSE(world.has<Velocity>(entity2));
}

TEST_F(CRUDTest, ComponentRemove)
{
	world.set<Position>(entity, { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(entity, { 10.0f, 20.0f, 30.0f });
	world.set<Health>(entity, { 100 });

	EXPECT_TRUE(world.has<Position>(entity));
	EXPECT_TRUE(world.has<Velocity>(entity));
	EXPECT_TRUE(world.has<Health>(entity));

	world.remove<Velocity>(entity);

	EXPECT_TRUE(world.has<Position>(entity));
	EXPECT_FALSE(world.has<Velocity>(entity));
	EXPECT_TRUE(world.has<Health>(entity));

	auto *vel_ptr = world.get<Velocity>(entity);
	EXPECT_EQ(vel_ptr, nullptr);

	auto *pos_ptr = world.get<Position>(entity);
	auto *health_ptr = world.get<Health>(entity);
	ASSERT_FALSE(pos_ptr == nullptr);
	ASSERT_FALSE(vel_ptr == nullptr);
	ASSERT_FALSE(health_ptr == nullptr);
	EXPECT_EQ(pos_ptr->x, 1.0f);
	EXPECT_EQ(health_ptr->value, 100);

	world.remove<Position>(entity);
	world.remove<Health>(entity);

	EXPECT_FALSE(world.has<Position>(entity));
	EXPECT_FALSE(world.has<Velocity>(entity));
	EXPECT_FALSE(world.has<Health>(entity));
}

TEST_F(CRUDTest, EntityLifecycle)
{
	world.set<Position>(entity, { 1.0f, 2.0f, 3.0f });
	world.set<Health>(entity, { 100 });

	EXPECT_TRUE(world.has<Position>(entity));
	EXPECT_TRUE(world.has<Health>(entity));

	world.despawn(entity);

	EXPECT_FALSE(world.has<Position>(entity));
	EXPECT_FALSE(world.has<Health>(entity));

	ncs::Entity new_entity = world.entity();

	world.set<Position>(new_entity, { 4.0f, 5.0f, 6.0f });

	EXPECT_TRUE(world.has<Position>(new_entity));
	EXPECT_FALSE(world.has<Position>(entity));
}

TEST_F(CRUDTest, MultipleEntities)
{
	const ncs::Entity entity1 = entity; /* `entity` is from `SetUp()`*/
	const ncs::Entity entity2 = world.entity();
	const ncs::Entity entity3 = world.entity();

	world.set<Position>(entity1, { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(entity1, { 10.0f, 20.0f, 30.0f });

	world.set<Position>(entity2, { 4.0f, 5.0f, 6.0f });
	world.set<Health>(entity2, { 200 });

	world.set<Velocity>(entity3, { 40.0f, 50.0f, 60.0f });
	world.set<Health>(entity3, { 300 });

	EXPECT_TRUE(world.has<Position>(entity1));
	EXPECT_TRUE(world.has<Velocity>(entity1));
	EXPECT_FALSE(world.has<Health>(entity1));

	EXPECT_TRUE(world.has<Position>(entity2));
	EXPECT_FALSE(world.has<Velocity>(entity2));
	EXPECT_TRUE(world.has<Health>(entity2));

	EXPECT_FALSE(world.has<Position>(entity3));
	EXPECT_TRUE(world.has<Velocity>(entity3));
	EXPECT_TRUE(world.has<Health>(entity3));

	auto *pos1 = world.get<Position>(entity1);
	ASSERT_NE(pos1, nullptr);
	EXPECT_EQ(pos1->y, 2.0f);

	auto *health2 = world.get<Health>(entity2);
	ASSERT_NE(health2, nullptr);
	EXPECT_EQ(health2->value, 200);

	auto *vel3 = world.get<Velocity>(entity3);
	ASSERT_NE(vel3, nullptr);
	EXPECT_EQ(vel3->z, 60.0f);
}

TEST_F(CRUDTest, NonTrivial)
{
	Name original("TestEntity");
	world.set<Name>(entity, original);

	EXPECT_TRUE(world.has<Name>(entity));

	auto* name_ptr = world.get<Name>(entity);
	ASSERT_NE(name_ptr, nullptr);
	EXPECT_EQ(name_ptr->name, "TestEntity");

	Name updated("UpdatedName");
	world.set<Name>(entity, updated);

	name_ptr = world.get<Name>(entity);
	ASSERT_NE(name_ptr, nullptr);
	EXPECT_EQ(name_ptr->name, "UpdatedName");

	name_ptr->name = "DirectlyModified";

	auto* retrieved_ptr = world.get<Name>(entity);
	ASSERT_NE(retrieved_ptr, nullptr);
	EXPECT_EQ(retrieved_ptr->name, "DirectlyModified");

	retrieved_ptr->name = "Prefix" + retrieved_ptr->name;

	auto* concat_ptr = world.get<Name>(entity);
	ASSERT_NE(concat_ptr, nullptr);
	EXPECT_EQ(concat_ptr->name, "PrefixDirectlyModified");

	world.remove<Name>(entity);
	EXPECT_FALSE(world.has<Name>(entity));

	const ncs::Entity entity2 = world.entity();
	world.set<Name>(entity, { "Entity1" });
	world.set<Name>(entity2, { "Entity2" });

	auto* name1 = world.get<Name>(entity);
	auto* name2 = world.get<Name>(entity2);

	ASSERT_NE(name1, nullptr);
	ASSERT_NE(name2, nullptr);
	EXPECT_EQ(name1->name, "Entity1");
	EXPECT_EQ(name2->name, "Entity2");

	// world.despawn(entity);
	// world.despawn(entity2);
}
