// This file is a part of the OpenSurgSim project.
// Copyright 2013, SimQuest Solutions Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <memory>

#include <SurgSim/DataStructures/OptionalValue.h>

namespace SurgSim
{
namespace DataStructures
{

TEST(OptionalValueTests, InitTest)
{
	EXPECT_NO_THROW({OptionalValue<int> a;});
	EXPECT_NO_THROW({OptionalValue<double> b(10.0);});

	OptionalValue<int> a;
	EXPECT_FALSE(a.hasValue());
}

TEST(OptionalValueTests, AssertTest)
{
	OptionalValue<std::shared_ptr<int>> a;

	EXPECT_ANY_THROW({ auto i = a.getValue();});
}


TEST(OptionalValueTests, SetValueTest)
{
	OptionalValue<double> a;
	EXPECT_FALSE(a.hasValue());
	a.setValue(10.0);

	EXPECT_TRUE(a.hasValue());
	EXPECT_EQ(10.0, a.getValue());

	a.invalidate();

	EXPECT_FALSE(a.hasValue());
}

TEST(OptionalValueTests, ComparatorTest)
{
	OptionalValue<int> a;
	OptionalValue<int> b;

	// Unassigned should be not equal
	EXPECT_TRUE(a == b);

	a.setValue(10);
	EXPECT_FALSE(a == b);

	b.setValue(10);
	EXPECT_TRUE(a == b);

	b.setValue(20);
	EXPECT_FALSE(a == b);

	a.invalidate();
	EXPECT_FALSE(a == b);
}


}; // namespace DataStructures
}; // namespace SurgSim
