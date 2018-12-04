#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	SetLens(0.25f * XM_PI, 1.0f, 1.0f, 1000.0f);
}

Camera::~Camera()
{
}

XMVECTOR Camera::GetPosition() const
{
	return XMLoadFloat3(&mPosition);
}

XMFLOAT3 Camera::GetPosition3f() const
{
	return mPosition;
}

void Camera::SetPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
	mViewDirty = true;
}

void Camera::SetPosition(const XMFLOAT3 & v)
{
	mPosition = v;
	mViewDirty = true;
}

XMVECTOR Camera::GetRight() const
{
	return XMLoadFloat3(&mRight);
}

XMFLOAT3 Camera::GetRight3f() const
{
	return mRight;
}

XMVECTOR Camera::GetUp() const
{
	return XMLoadFloat3(&mUp);
}

XMFLOAT3 Camera::GetUp3f() const
{
	return mUp;
}

XMVECTOR Camera::GetLook() const
{
	return XMLoadFloat3(&mLook);
}

XMFLOAT3 Camera::GetLook3f() const
{
	return mLook;
}


float Camera::GetNearZ()const
{
	return mNearZ;
}

float Camera::GetFarZ()const
{
	return mFarZ;
}

float Camera::GetAspect()const
{
	return mAspect;
}

float Camera::GetFovY()const
{
	return mFovY;
}

float Camera::GetFovX()const
{
	float halfWidth = 0.5f * GetNearWindowWidth();
	return 2.0f * atan(halfWidth / mNearZ);
}

float Camera::GetNearWindowWidth()const
{
	return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight()const
{
	return mNearWindowHeight;
}

float Camera::GetFarWindowWidth()const
{
	return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight()const
{
	return mFarWindowHeight;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	XMMATRIX p = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, p);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);

	mViewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	mViewDirty = true;
}

XMMATRIX Camera::GetView()const
{

}

XMMATRIX Camera::GetProj()const
{

}

XMFLOAT4X4 Camera::GetView4x4f()const
{

}

XMFLOAT4X4 Camera::GetProj4x4f()const
{

}

void Camera::Strafe(float d)
{

}

void Camera::Walk(float d)
{

}
	 
void Camera::Pitch(float angle)
{

}

void Camera::RotateY(float angle)
{

}
	
void Camera::UpdateViewMatrix()
{

}