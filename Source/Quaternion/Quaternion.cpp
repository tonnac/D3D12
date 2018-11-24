
#include <DirectXMath.h>

using namespace DirectX;

int main()
{
	XMVECTOR dot = XMQuaternionDot(XMVECTOR({ 1.0f,1.0f,1.0f,3.0f }), XMVECTOR({1.0f,1.0f,1.0f,1.0f}));
	XMVECTOR rot = XMQuaternionRotationNormal(XMVECTOR{ 0,1,0,0 }, 30);

	XMVECTOR r1 = XMQuaternionRotationAxis(XMVECTOR{ 1,1,1 }, XM_PI);

	XMFLOAT4 z2;
	XMStoreFloat4(&z2, r1);

	XMVECTOR p = XMVectorSet(1, 1, 1, cosf(XM_PIDIV2));
	XMVECTOR p1 = XMQuaternionNormalize(p);
	XMVECTOR p2 = XMQuaternionNormalize(r1);

	XMVECTOR l = XMVectorSet(10, 5, 23, cosf(XM_PIDIV2));
	XMVECTOR l2 = XMQuaternionNormalize(l);

	bool ppl = XMQuaternionEqual(p2, p1);

	return 0;
}