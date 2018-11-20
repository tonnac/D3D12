
#include <DirectXMath.h>

using namespace DirectX;

int main()
{
	XMVECTOR dot = XMQuaternionDot(XMVECTOR({ 1.0f,1.0f,1.0f,3.0f }), XMVECTOR({1.0f,1.0f,1.0f,1.0f}));
	XMVECTOR rot = XMQuaternionRotationNormal(XMVECTOR{ 0,1,0,0 }, 30);

	return 0;
}