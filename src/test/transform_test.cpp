// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gtest/gtest.h"

#include "ppx/transform.h"

using namespace ppx;

TEST(TransformTest, Identity)
{
    Transform transform;

    EXPECT_EQ(transform.GetTranslation(), float3(0, 0, 0));
    EXPECT_EQ(transform.GetScale(), float3(1, 1, 1));
    EXPECT_EQ(transform.GetRotation(), float3(0, 0, 0));
    EXPECT_EQ(transform.GetRotationOrder(), Transform::RotationOrder::XYZ);

    EXPECT_EQ(transform.GetTranslationMatrix(), glm::translate(float3(0, 0, 0)));
    EXPECT_EQ(transform.GetScaleMatrix(), glm::scale(float3(1, 1, 1)));
    EXPECT_EQ(transform.GetRotationMatrix(), glm::eulerAngleXYZ(0.0f, 0.0f, 0.0f));
}

TEST(TransformTest, Scale)
{
    Transform transform;
    transform.SetScale(float3(3, 5, 7));
    EXPECT_EQ(transform.GetScale(), float3(3, 5, 7));
    EXPECT_EQ(transform.GetScaleMatrix(), glm::scale(float3(3, 5, 7)));
}

TEST(TransformTest, Translate)
{
    Transform transform;
    transform.SetTranslation(float3(3, 5, 7));
    EXPECT_EQ(transform.GetTranslation(), float3(3, 5, 7));
    EXPECT_EQ(transform.GetTranslationMatrix(), glm::translate(float3(3, 5, 7)));
}

TEST(TransformTest, Rotate)
{
    Transform transform;
    transform.SetRotation(float3(3, 5, 7));
    EXPECT_EQ(transform.GetRotation(), float3(3, 5, 7));
    EXPECT_EQ(transform.GetRotationMatrix(), glm::eulerAngleXYZ(3.0f, 5.0f, 7.0f));
}

TEST(TransformTest, TranslateScaleRotate)
{
    Transform transform;
    transform.SetTranslation(float3(19, 23, 29));
    transform.SetScale(float3(11, 13, 17));
    transform.SetRotation(float3(3, 5, 7));
    EXPECT_EQ(transform.GetConcatenatedMatrix(), glm::translate(float3(19, 23, 29)) * glm::eulerAngleXYZ(3.0f, 5.0f, 7.0f) * glm::scale(float3(11, 13, 17)));
}
