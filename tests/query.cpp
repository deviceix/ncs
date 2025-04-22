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

struct Tag1 {};

struct Tag2 {};

struct Tag3 {};

TEST(WorldTest, EntityCountIteration)
{
	ncs::World world;

	for (auto i = 0; i < 5; ++i)
	{
		const auto entity = world.entity();
		world.set<Position>(entity, Position { static_cast<float>(i), 0.0f, 0.0f });
		world.set<Velocity>(entity, Velocity { 0.0f, static_cast<float>(i), 0.0f });
	}

	auto q = world.query<Position, Velocity>();
	EXPECT_EQ(q.size(), 5);
}

TEST(WorldTest, MixedComponents)
{
	ncs::World world;

	const auto e1 = world.entity();
	const auto e2 = world.entity();
	const auto e3 = world.entity();
	const auto e4 = world.entity();

	world.set<Position>(e1, { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(e1, { 10.0f, 20.0f, 30.0f });

	world.set<Position>(e2, { 4.0f, 5.0f, 6.0f });
	world.set<Health>(e2, { 100 });

	world.set<Velocity>(e3, { 7.0f, 8.0f, 9.0f });
	world.set<Health>(e3, { 200 });

	world.set<Position>(e4, { 11.0f, 12.0f, 13.0f });
	world.set<Velocity>(e4, { 14.0f, 15.0f, 16.0f });
	world.set<Health>(e4, { 300 });

	const auto q1 = world.query<Position>();
	EXPECT_EQ(q1.size(), 3); /* e1, e2, e4 */

	const auto q2 = world.query<Position, Velocity>();
	EXPECT_EQ(q2.size(), 2); /* e1, e4 */

	const auto q3 = world.query<Position, Health>();
	EXPECT_EQ(q3.size(), 2); /* e2, e4 */

	const auto q4 = world.query<Position, Velocity, Health>();
	EXPECT_EQ(q4.size(), 1); /* only e4 */
}

TEST(WorldTest, ComponentDataAccess)
{
	ncs::World world;

	const auto entity = world.entity();
	world.set<Position>(entity, { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(entity, { 10.0f, 20.0f, 30.0f });

	auto q = world.query<Position, Velocity>();
	EXPECT_EQ(q.size(), 1);

	auto& [e, pos, vel] = q[0];
	EXPECT_FALSE(pos == nullptr);
	EXPECT_FALSE(vel == nullptr);
	EXPECT_EQ(pos->x, 1.0f);
	EXPECT_EQ(pos->y, 2.0f);
	EXPECT_EQ(pos->z, 3.0f);
	EXPECT_EQ(vel->x, 10.0f);
	EXPECT_EQ(vel->y, 20.0f);
	EXPECT_EQ(vel->z, 30.0f);
}

TEST(WorldTest, QueryAfterModification)
{
	ncs::World world;

	const auto e1 = world.entity();
	const auto e2 = world.entity();

	world.set<Position>(e1, { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(e1, { 10.0f, 20.0f, 30.0f });

	world.set<Position>(e2, Position { 4.0f, 5.0f, 6.0f });

	auto q1 = world.query<Position, Velocity>();
	EXPECT_EQ(q1.size(), 1); /* only e1 */

	world.set<Velocity>(e2, { 40.0f, 50.0f, 60.0f });

	auto q2 = world.query<Position, Velocity>();
	EXPECT_EQ(q2.size(), 2); /* e1 & e2 */
	world.remove<Velocity>(e1);

	auto q3 = world.query<Position, Velocity>();
	EXPECT_EQ(q3.size(), 1); /* only e2 */
}

TEST(WorldTest, QueryAfterDespawn)
{
	ncs::World world;

	const auto e1 = world.entity();
	const auto e2 = world.entity();

	world.set<Position>(e1, Position { 1.0f, 2.0f, 3.0f });
	world.set<Position>(e2, Position { 4.0f, 5.0f, 6.0f });

	auto q1 = world.query<Position>();
	EXPECT_EQ(q1.size(), 2);

	world.despawn(e1);

	auto q2 = world.query<Position>();
	EXPECT_EQ(q2.size(), 1);

	auto& [e, pos] = q2[0];
	EXPECT_EQ(pos->x, 4.0f);
	EXPECT_EQ(pos->y, 5.0f);
	EXPECT_EQ(pos->z, 6.0f);
}

TEST(WorldTest, QuerySameArchetype)
{
	ncs::World world;
	std::vector<ncs::Entity> entities;
	for (auto i = 0; i < 10; ++i)
	{
		auto e = world.entity();
		entities.push_back(e);
		world.set<Position>(e, Position { static_cast<float>(i), 0.0f, 0.0f });
		world.set<Velocity>(e, Velocity { 0.0f, static_cast<float>(i), 0.0f });
		world.set<Health>(e, Health { i * 10 });
	}

	const auto q1 = world.query<Position>();
	EXPECT_EQ(q1.size(), 10);

	const auto q2 = world.query<Velocity>();
	EXPECT_EQ(q2.size(), 10);

	const auto q3 = world.query<Health>();
	EXPECT_EQ(q3.size(), 10);

	const auto q4 = world.query<Position, Velocity>();
	EXPECT_EQ(q4.size(), 10);

	const auto q5 = world.query<Position, Health>();
	EXPECT_EQ(q5.size(), 10);

	const auto q6 = world.query<Velocity, Health>();
	EXPECT_EQ(q6.size(), 10);

	const auto q7 = world.query<Position, Velocity, Health>();
	EXPECT_EQ(q7.size(), 10);
}

TEST(WorldTest, QueryEmptyArchetype)
{
	ncs::World world;

	/* create an entity; then despawn */
	const auto e = world.entity();
	world.set<Position>(e, Position { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(e, Velocity { 10.0f, 20.0f, 30.0f });
	world.despawn(e);

	/* should empty; even if archetype exists (the arch is empty) */
	const auto q = world.query<Position, Velocity>();
	EXPECT_EQ(q.size(), 0);
}

TEST(WorldTest, ModifyDuringIter)
{
	ncs::World world;

	std::vector<ncs::Entity> entities;
	for (auto i = 0; i < 5; ++i)
	{
		auto e = world.entity();
		entities.push_back(e);
		world.set<Position>(e, Position { static_cast<float>(i), 0.0f, 0.0f });
	}

	for (auto &[entity, pos]: world.query<Position>())
		world.set<Velocity>(entity, { pos->x, pos->y, pos->z });

	/* should now have 5 entities */
	const auto q = world.query<Position, Velocity>();
	EXPECT_EQ(q.size(), 5);
}

TEST(WorldTest, ComponentOrderInQuery)
{
	ncs::World world;

	auto e = world.entity();
	world.set<Position>(e, Position { 1.0f, 2.0f, 3.0f });
	world.set<Velocity>(e, Velocity { 10.0f, 20.0f, 30.0f });

	/* different order; shouldn't affect anything at all */
	auto q1 = world.query<Position, Velocity>();
	auto q2 = world.query<Velocity, Position>();

	EXPECT_EQ(q1.size(), 1);
	EXPECT_EQ(q2.size(), 1);

	auto [e1, pos1, vel1] = q1[0];
	auto [e2, vel2, pos2] = q2[0];

	EXPECT_EQ(pos1->x, 1.0f);
	EXPECT_EQ(vel1->x, 10.0f);
	EXPECT_EQ(pos2->x, 1.0f);
	EXPECT_EQ(vel2->x, 10.0f);
}

TEST(WorldTest, MultipleArchetype)
{
	ncs::World world;

	/* different archetype with position */
	for (auto i = 0; i < 5; ++i)
	{
		const auto e = world.entity();
		world.set<Position>(e, Position { static_cast<float>(i), 0.0f, 0.0f });
	}

	for (auto i = 0; i < 3; ++i)
	{
		const auto e = world.entity();
		world.set<Position>(e, Position { static_cast<float>(i), 1.0f, 0.0f });
		world.set<Velocity>(e, Velocity { static_cast<float>(i), 0.0f, 0.0f });
	}

	for (auto i = 0; i < 2; ++i)
	{
		const auto e = world.entity();
		world.set<Position>(e, Position { static_cast<float>(i), 2.0f, 0.0f });
		world.set<Health>(e, Health { i * 10 });
	}

	for (auto i = 0; i < 4; ++i)
	{
		const auto e = world.entity();
		world.set<Position>(e, Position { static_cast<float>(i), 3.0f, 0.0f });
		world.set<Velocity>(e, Velocity { static_cast<float>(i), 1.0f, 0.0f });
		world.set<Health>(e, Health { i * 20 });
	}

	const auto q1 = world.query<Position>();
	EXPECT_EQ(q1.size(), 5 + 3 + 2 + 4);

	/* q pv */
	const auto q2 = world.query<Position, Velocity>();
	EXPECT_EQ(q2.size(), 3 + 4);

	/* q pos n hp */
	const auto q3 = world.query<Position, Health>();
	EXPECT_EQ(q3.size(), 2 + 4);

	/* q all */
	const auto q4 = world.query<Position, Velocity, Health>();
	EXPECT_EQ(q4.size(), 4);
}

TEST(WorldTest, LargeQuery)
{
	ncs::World world;
	for (auto i = 0; i < 1000; ++i)
	{
		const auto e = world.entity();
		world.set<Position>(e, Position { static_cast<float>(i), 0.0f, 0.0f });

		/* every third entity also gets Velocity */
		if (i % 3 == 0)
			world.set<Velocity>(e, { 0.0f, static_cast<float>(i), 0.0f });

		/* every fifth entity has health component */
		if (i % 5 == 0)
			world.set<Health>(e, { i });
	}

	/* query for Position (all entities) */
	const auto q1 = world.query<Position>();
	EXPECT_EQ(q1.size(), 1000);

	/* query position and velocity (every third) */
	const auto q2 = world.query<Position, Velocity>();
	EXPECT_EQ(q2.size(), 1000 / 3 + (1000 % 3 > 0 ? 1 : 0));

	/* query pos and health (every fifth) */
	const auto q3 = world.query<Position, Health>();
	EXPECT_EQ(q3.size(), 1000 / 5 + (1000 % 5 > 0 ? 1 : 0));

	/* query for all three (every fifteenth) */
	const auto q4 = world.query<Position, Velocity, Health>();
	EXPECT_EQ(q4.size(), 1000 / 15 + (1000 % 15 > 0 ? 1 : 0));
}
