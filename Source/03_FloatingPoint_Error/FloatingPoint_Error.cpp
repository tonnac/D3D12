#include "FloatingPoint_Error.h"

int main()
{
	cout.precision(8);

	if (!XMVerifyCPUSupport())
	{
		return 0;
	}

	XMVECTOR u = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMVECTOR n = XMVector3Normalize(u);
	XMVECTOR Eps = XMVectorReplicate(Epsilon);

	float LU = XMVectorGetX(XMVector3Length(n));

	XMVECTOR lu = XMVectorSet(XMVectorGetX(n) / LU, 
		XMVectorGetY(n) / LU, XMVectorGetZ(n) / LU, 0.0f);

	cout << LU << endl;
	if (LU == 1.0f)
		cout << "Length : 1" << endl;
	else
		cout << "Length is not 1" << endl;

	float powLU = powf(LU, 1.0e6f);
	cout << "LU^(10^6) = " << powLU << endl;

	

	XMVector3NearEqual(lu, n, Eps);


	return 0;
}