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

#include "ppx/camera.h"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// Camera
// -------------------------------------------------------------------------------------------------
Camera::Camera(bool pixelAligned)
    : mPixelAligned(pixelAligned)
{
    LookAt(PPX_CAMERA_DEFAULT_EYE_POSITION, PPX_CAMERA_DEFAULT_LOOK_AT, PPX_CAMERA_DEFAULT_WORLD_UP);
}

Camera::Camera(float nearClip, float farClip, bool pixelAligned)
    : mPixelAligned(pixelAligned),
      mNearClip(nearClip),
      mFarClip(farClip)
{
}

void Camera::LookAt(const float3& eye, const float3& target, const float3& up)
{
    const float3 yAxis    = mPixelAligned ? float3(1, -1, 1) : float3(1, 1, 1);
    mEyePosition          = eye;
    mTarget               = target;
    mWorldUp              = up;
    mViewDirection        = glm::normalize(mTarget - mEyePosition);
    mViewMatrix           = glm::scale(yAxis) * glm::lookAt(mEyePosition, mTarget, mWorldUp);
    mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    mInverseViewMatrix    = glm::inverse(mViewMatrix);
}

float3 Camera::WorldToViewPoint(const float3& worldPoint) const
{
    float3 viewPoint = float3(mViewMatrix * float4(worldPoint, 1.0f));
    return viewPoint;
}

float3 Camera::WorldToViewVector(const float3& worldVector) const
{
    float3 viewPoint = float3(mViewMatrix * float4(worldVector, 0.0f));
    return viewPoint;
}

void Camera::MoveAlongViewDirection(float distance)
{
    float3 eyePosition = mEyePosition + (distance * mViewDirection);
    LookAt(eyePosition, mTarget, mWorldUp);
}

// -------------------------------------------------------------------------------------------------
// PerspCamera
// -------------------------------------------------------------------------------------------------
PerspCamera::PerspCamera()
{
}

PerspCamera::PerspCamera(
    float horizFovDegrees,
    float aspect,
    float nearClip,
    float farClip)
    : Camera(nearClip, farClip)
{
    SetPerspective(
        horizFovDegrees,
        aspect,
        nearClip,
        farClip);
}

PerspCamera::PerspCamera(
    const float3& eye,
    const float3& target,
    const float3& up,
    float         horizFovDegrees,
    float         aspect,
    float         nearClip,
    float         farClip)
    : Camera(nearClip, farClip)
{
    LookAt(eye, target, up);

    SetPerspective(
        horizFovDegrees,
        aspect,
        nearClip,
        farClip);
}

PerspCamera::PerspCamera(
    uint32_t pixelWidth,
    uint32_t pixelHeight,
    float    horizFovDegrees)
    : Camera(true)
{
    float aspect   = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
    float eyeX     = pixelWidth / 2.0f;
    float eyeY     = pixelHeight / 2.0f;
    float halfFov  = atan(tan(glm::radians(horizFovDegrees) / 2.0f) / aspect); // horiz fov -> vert fov
    float theTan   = tanf(halfFov);
    float dist     = eyeY / theTan;
    float nearClip = dist / 10.0f;
    float farClip  = dist * 10.0f;

    SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
    LookAt(float3(eyeX, eyeY, dist), float3(eyeX, eyeY, 0.0f));
}

PerspCamera::PerspCamera(
    uint32_t pixelWidth,
    uint32_t pixelHeight,
    float    horizFovDegrees,
    float    nearClip,
    float    farClip)
    : Camera(nearClip, farClip, true)
{
    float aspect  = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
    float eyeX    = pixelWidth / 2.0f;
    float eyeY    = pixelHeight / 2.0f;
    float halfFov = atan(tan(glm::radians(horizFovDegrees) / 2.0f) / aspect); // horiz fov -> vert fov
    float theTan  = tanf(halfFov);
    float dist    = eyeY / theTan;

    SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
    LookAt(float3(eyeX, eyeY, dist), float3(eyeX, eyeY, 0.0f));
}

PerspCamera::~PerspCamera()
{
}

void PerspCamera::SetPerspective(
    float horizFovDegrees,
    float aspect,
    float nearClip,
    float farClip)
{
    mHorizFovDegrees = horizFovDegrees;
    mAspect          = aspect;
    mNearClip        = nearClip;
    mFarClip         = farClip;

    float horizFovRadians = glm::radians(mHorizFovDegrees);
    float vertFovRadians  = 2.0f * atan(tan(horizFovRadians / 2.0f) / mAspect);
    mVertFovDegrees       = glm::degrees(vertFovRadians);

    mProjectionMatrix = glm::perspective(
        vertFovRadians,
        mAspect,
        mNearClip,
        mFarClip);

    mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}

void PerspCamera::FitToBoundingBox(const float3& bboxMinWorldSpace, const float3& bbxoMaxWorldSpace)
{
    float3   min             = bboxMinWorldSpace;
    float3   max             = bbxoMaxWorldSpace;
    float3   target          = (min + max) / 2.0f;
    float3   up              = glm::normalize(mInverseViewMatrix * float4(0, 1, 0, 0));
    float4x4 viewSpaceMatrix = glm::lookAt(mEyePosition, target, up);

    // World space oriented bounding box
    float3 obb[8] = {
        {min.x, max.y, min.z},
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {max.x, max.y, min.z},
        {min.x, max.y, max.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, max.z},
    };

    // Tranform obb from world space to view space
    for (uint32_t i = 0; i < 8; ++i) {
        obb[i] = viewSpaceMatrix * float4(obb[i], 1.0f);
    }

    // Get aabb from obb in view space
    min = max = obb[0];
    for (uint32_t i = 1; i < 8; ++i) {
        min = glm::min(min, obb[i]);
        max = glm::max(max, obb[i]);
    }

    // Get x,y extent max
    float xmax = glm::max(glm::abs(min.x), glm::abs(max.x));
    float ymax = glm::max(glm::abs(min.y), glm::abs(max.y));
    float rad  = glm::max(xmax, ymax);
    float fov  = mAspect < 1.0f ? mHorizFovDegrees : mVertFovDegrees;

    // Calculate distance
    float dist = rad / tan(glm::radians(fov / 2.0f));

    // Calculate eye position
    float3 dir = glm::normalize(mEyePosition - target);
    float3 eye = target + (dist + mNearClip) * dir;

    // Adjust camera look at
    LookAt(eye, target, up);
}

// -------------------------------------------------------------------------------------------------
// OrthoCamera
// -------------------------------------------------------------------------------------------------
OrthoCamera::OrthoCamera()
{
}

OrthoCamera::OrthoCamera(
    float left,
    float right,
    float bottom,
    float top,
    float near_clip,
    float far_clip)
{
    SetOrthographic(
        left,
        right,
        bottom,
        top,
        near_clip,
        far_clip);
}

OrthoCamera::~OrthoCamera()
{
}

void OrthoCamera::SetOrthographic(
    float left,
    float right,
    float bottom,
    float top,
    float nearClip,
    float farClip)
{
    mLeft     = left;
    mRight    = right;
    mBottom   = bottom;
    mTop      = top;
    mNearClip = nearClip;
    mFarClip  = farClip;

    mProjectionMatrix = glm::ortho(
        mLeft,
        mRight,
        mBottom,
        mTop,
        mNearClip,
        mFarClip);
}

// -------------------------------------------------------------------------------------------------
// ArcballCamera
// -------------------------------------------------------------------------------------------------
ArcballCamera::ArcballCamera()
{
}

ArcballCamera::ArcballCamera(
    float horizFovDegrees,
    float aspect,
    float nearClip,
    float farClip)
    : PerspCamera(horizFovDegrees, aspect, nearClip, farClip)
{
}

ArcballCamera::ArcballCamera(
    const float3& eye,
    const float3& target,
    const float3& up,
    float         horizFovDegrees,
    float         aspect,
    float         nearClip,
    float         farClip)
    : PerspCamera(eye, target, up, horizFovDegrees, aspect, nearClip, farClip)
{
}

void ArcballCamera::UpdateCamera()
{
    mViewMatrix        = mTranslationMatrix * glm::mat4_cast(mRotationQuat) * mCenterTranslationMatrix;
    mInverseViewMatrix = glm::inverse(mViewMatrix);

    // Transform the view space origin into world space for eye position
    mEyePosition = mInverseViewMatrix * float4(0, 0, 0, 1);
}

void ArcballCamera::LookAt(const float3& eye, const float3& target, const float3& up)
{
    Camera::LookAt(eye, target, up);

    float3 viewDir = target - eye;
    float3 zAxis   = glm::normalize(viewDir);
    float3 xAxis   = glm::normalize(glm::cross(zAxis, glm::normalize(up)));
    float3 yAxis   = glm::normalize(glm::cross(xAxis, zAxis));
    xAxis          = glm::normalize(glm::cross(zAxis, yAxis));

    mCenterTranslationMatrix = glm::inverse(glm::translate(target));
    mTranslationMatrix       = glm::translate(float3(0.0f, 0.0f, -glm::length(viewDir)));
    mRotationQuat            = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(xAxis, yAxis, -zAxis))));

    UpdateCamera();
}

static quat ScreenToArcball(const float2& p)
{
    float dist = glm::dot(p, p);

    // If we're on/in the sphere return the point on it
    if (dist <= 1.0f) {
        return glm::quat(0.0f, p.x, p.y, glm::sqrt(1.0f - dist));
    }

    // Otherwise we project the point onto the sphere
    const glm::vec2 proj = glm::normalize(p);
    return glm::quat(0.0f, proj.x, proj.y, 0.0f);
}

void ArcballCamera::Rotate(const float2& prevPos, const float2& curPos)
{
    const float2 kNormalizeDeviceCoordinatesMin = float2(-1, -1);
    const float2 kNormalizeDeviceCoordinatesMax = float2(1, 1);

    // Clamp mouse positions to stay in NDC
    float2 clampedCurPos  = glm::clamp(curPos, kNormalizeDeviceCoordinatesMin, kNormalizeDeviceCoordinatesMax);
    float2 clampedPrevPos = glm::clamp(prevPos, kNormalizeDeviceCoordinatesMin, kNormalizeDeviceCoordinatesMax);

    quat mouseCurBall  = ScreenToArcball(clampedCurPos);
    quat mousePrevBall = ScreenToArcball(clampedPrevPos);

    mRotationQuat = mouseCurBall * mousePrevBall * mRotationQuat;

    UpdateCamera();
}

void ArcballCamera::Pan(const float2& delta)
{
    float  zoomAmount = glm::abs(mTranslationMatrix[3][2]);
    float4 motion     = float4(delta.x * zoomAmount, delta.y * zoomAmount, 0.0f, 0.0f);

    // Find the panning amount in the world space
    motion = mInverseViewMatrix * motion;

    mCenterTranslationMatrix = glm::translate(float3(motion)) * mCenterTranslationMatrix;

    UpdateCamera();
}

void ArcballCamera::Zoom(float amount)
{
    float3 motion = float3(0.0f, 0.0f, amount);

    mTranslationMatrix = glm::translate(motion) * mTranslationMatrix;

    UpdateCamera();
}

} // namespace ppx
