#include <unordered_set>
#include <gtest/gtest.h>
#include <ncs/world.hpp>

class LifecycleTest : public testing::Test
{
protected:
	void SetUp() override {}

	void TearDown() override {}

	ncs::World world;
};

TEST_F(LifecycleTest, Multiple)
{
	std::vector<ncs::Entity> entities;
	entities.reserve(100);

	for (auto i = 0; i < 100; ++i)
		entities.emplace_back(world.entity());

	std::unordered_set<uint64_t> unique_ids;
	for (const auto entity : entities)
		unique_ids.insert(ncs::World::get_eid(entity));

	EXPECT_EQ(unique_ids.size(), 100);
}

TEST_F(LifecycleTest, Recyling)
{
	const auto first = world.entity();
	world.despawn(first);
	const auto recycled = world.entity();

	/* recycled entity should have different generation but same underlying id */
	EXPECT_EQ(world.get_eid(first), world.get_eid(recycled));
	EXPECT_NE(world.get_egen(first), world.get_egen(recycled));
}

TEST_F(LifecycleTest, Respawn)
{
	const auto e1 = world.entity();
	const auto e2 = world.entity();
	const auto e3 = world.entity();

	world.despawn(e1);
	world.despawn(e2);
	world.despawn(e3);

	const auto reused = world.entity();

	EXPECT_EQ(world.get_eid(reused), world.get_eid(e3));
	EXPECT_NE(world.get_egen(reused), world.get_egen(e3));
}
