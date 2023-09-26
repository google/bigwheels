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

#ifndef ppx_camera_h
#define ppx_camera_h

#include "ppx/math_config.h"

#define PPX_CAMERA_DEFAULT_NEAR_CLIP      0.1f
#define PPX_CAMERA_DEFAULT_FAR_CLIP       10000.0f
#define PPX_CAMERA_DEFAULT_EYE_POSITION   float3(0, 0, 1)
#define PPX_CAMERA_DEFAULT_LOOK_AT        float3(0, 0, 0)
#define PPX_CAMERA_DEFAULT_WORLD_UP       float3(0, 1, 0)
#define PPX_CAMERA_DEFAULT_VIEW_DIRECTION float3(0, 0, -1)

namespace ppx {

enum CameraType
{
    CAMERA_TYPE_UNKNOWN      = 0,
    CAMERA_TYPE_PERSPECTIVE  = 1,
    CAMERA_TYPE_ORTHOGRAPHIC = 2,
};

// -------------------------------------------------------------------------------------------------
// Camera
// -------------------------------------------------------------------------------------------------
class Camera
{
public:
    Camera(bool pixelAligned = false);

    Camera(float nearClip, float farClip, bool pixelAligned = false);

    virtual ~Camera() {}

    virtual ppx::CameraType GetCameraType() const = 0;

    virtual void LookAt(const float3& eye, const float3& target, const float3& up = PPX_CAMERA_DEFAULT_WORLD_UP);

    const float3& GetEyePosition() const { return mEyePosition; }
    const float3& GetTarget() const { return mTarget; }
    const float3& GetViewDirection() const { return mViewDirection; }
    const float3& GetWorldUp() const { return mWorldUp; }

    const float4x4& GetViewMatrix() const { return mViewMatrix; }
    const float4x4& GetProjectionMatrix() const { return mProjectionMatrix; }
    const float4x4& GetViewProjectionMatrix() const { return mViewProjectionMatrix; }

    float3 WorldToViewPoint(const float3& worldPoint) const;
    float3 WorldToViewVector(const float3& worldVector) const;

    void MoveAlongViewDirection(float distance);

protected:
    bool             mPixelAligned         = false;
    float            mAspect               = 0;
    float            mNearClip             = PPX_CAMERA_DEFAULT_NEAR_CLIP;
    float            mFarClip              = PPX_CAMERA_DEFAULT_FAR_CLIP;
    float3           mEyePosition          = PPX_CAMERA_DEFAULT_EYE_POSITION;
    float3           mTarget               = PPX_CAMERA_DEFAULT_LOOK_AT;
    float3           mViewDirection        = PPX_CAMERA_DEFAULT_VIEW_DIRECTION;
    float3           mWorldUp              = PPX_CAMERA_DEFAULT_WORLD_UP;
    mutable float4x4 mViewMatrix           = float4x4(1);
    mutable float4x4 mProjectionMatrix     = float4x4(1);
    mutable float4x4 mViewProjectionMatrix = float4x4(1);
    mutable float4x4 mInverseViewMatrix    = float4x4(1);
};

// -------------------------------------------------------------------------------------------------
// PerspCamera
// -------------------------------------------------------------------------------------------------
class PerspCamera
    : public Camera
{
public:
    PerspCamera();

    explicit PerspCamera(
        float horizFovDegrees,
        float aspect,
        float nearClip = PPX_CAMERA_DEFAULT_NEAR_CLIP,
        float farClip  = PPX_CAMERA_DEFAULT_FAR_CLIP);

    explicit PerspCamera(
        const float3& eye,
        const float3& target,
        const float3& up,
        float         horizFovDegrees,
        float         aspect,
        float         nearClip = PPX_CAMERA_DEFAULT_NEAR_CLIP,
        float         farClip  = PPX_CAMERA_DEFAULT_FAR_CLIP);

    explicit PerspCamera(
        uint32_t pixelWidth,
        uint32_t pixelHeight,
        float    horizFovDegrees = 60.0f);

    // Pixel aligned camera
    explicit PerspCamera(
        uint32_t pixelWidth,
        uint32_t pixelHeight,
        float    horizFovDegrees,
        float    nearClip,
        float    farClip);

    virtual ~PerspCamera();

    virtual ppx::CameraType GetCameraType() const { return ppx::CAMERA_TYPE_PERSPECTIVE; }

    void SetPerspective(
        float horizFovDegrees,
        float aspect,
        float nearClip = PPX_CAMERA_DEFAULT_NEAR_CLIP,
        float farClip  = PPX_CAMERA_DEFAULT_FAR_CLIP);

    void FitToBoundingBox(const float3& bboxMinWorldSpace, const float3& bbxoMaxWorldSpace);

private:
    float mHorizFovDegrees = 60.0f;
    float mVertFovDegrees  = 36.98f;
    float mAspect          = 1.0f;
};

// -------------------------------------------------------------------------------------------------
// OrthoCamera
// -------------------------------------------------------------------------------------------------
class OrthoCamera
    : public Camera
{
public:
    OrthoCamera();

    OrthoCamera(
        float left,
        float right,
        float bottom,
        float top,
        float nearClip,
        float farClip);

    virtual ~OrthoCamera();

    virtual ppx::CameraType GetCameraType() const { return ppx::CAMERA_TYPE_ORTHOGRAPHIC; }

    void SetOrthographic(
        float left,
        float right,
        float bottom,
        float top,
        float nearClip,
        float farClip);

private:
    float mLeft   = -1.0f;
    float mRight  = 1.0f;
    float mBottom = -1.0f;
    float mTop    = 1.0f;
};

// -------------------------------------------------------------------------------------------------
// ArcballCamera
// -------------------------------------------------------------------------------------------------

//! @class ArcballCamera
//!
//! Adapted from: https://github.com/Twinklebear/arcball-cpp
//!
class ArcballCamera
    : public PerspCamera
{
public:
    ArcballCamera();

    ArcballCamera(
        float horizFovDegrees,
        float aspect,
        float nearClip = PPX_CAMERA_DEFAULT_NEAR_CLIP,
        float farClip  = PPX_CAMERA_DEFAULT_FAR_CLIP);

    ArcballCamera(
        const float3& eye,
        const float3& target,
        const float3& up,
        float         horizFovDegrees,
        float         aspect,
        float         nearClip = PPX_CAMERA_DEFAULT_NEAR_CLIP,
        float         farClip  = PPX_CAMERA_DEFAULT_FAR_CLIP);

    virtual ~ArcballCamera() {}

    void LookAt(const float3& eye, const float3& target, const float3& up = PPX_CAMERA_DEFAULT_WORLD_UP) override;

    //! @fn void Rotate(const float2& prevPos, const float2& curPos)
    //!
    //! @param prevPos previous mouse position in normalized device coordinates
    //! @param curPos current mouse position in normalized device coordinates
    //!
    void Rotate(const float2& prevPos, const float2& curPos);

    //! @fn void Pan(const float2& delta)
    //!
    //! @param delta mouse delta in normalized device coordinates
    //!
    void Pan(const float2& delta);

    //! @fn void Zoom(float amount)
    //!
    void Zoom(float amount);

    ////! @fn const float4x4& GetCameraMatrix() const
    ////!
    // const float4x4& GetCameraMatrix() const { return mCameraMatrix; }

    ////! @fn const float4x4& GetInverseCameraMatrix() const
    ////!
    // const float4x4& GetInverseCameraMatrix() const { return mInverseCameraMatrix; }

private:
    void UpdateCamera();

private:
    float4x4 mCenterTranslationMatrix;
    float4x4 mTranslationMatrix;
    quat     mRotationQuat;
    // float4x4 mCameraMatrix;
    // float4x4 mInverseCameraMatrix;
};

} // namespace ppx

#endif // ppx_camera_h
