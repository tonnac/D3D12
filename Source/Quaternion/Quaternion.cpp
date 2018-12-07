
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

	XMFLOAT4 ep;
	XMStoreFloat4(&ep, XMVectorReciprocal(XMVectorSet(0.1f, 0.2f, 0.3f, 0.4f)));

	XMVECTOR zero = XMVectorZero();

	XMVECTOR one = XMVectorSplatOne();
	XMVECTOR zero1 = XMVectorZero();
	XMVECTOR inf = XMVectorSplatInfinity();

	XMVECTOR v1 = XMVectorSet(1.0f, 9.0f, 12.0f, 2.0f);
	XMVECTOR v2 = XMVectorSet(5.0f, 7.0f, 9.0f, 11.0f);

	XMVECTOR ret = XMVectorGreater(v2, v1);

	XMVECTOR select = XMVectorSelect(v1, v2, v2);

	return 0;
}