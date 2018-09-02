#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;


// 부동소수점의 오차를 생각하여 작은 수인 epsilon을 정한다.

const float Epsilon = 0.001f;

bool Equals(float lhs, float rhs)
{
	return fabs(lhs - rhs) < Epsilon ? true : false;
}