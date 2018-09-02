#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;


// �ε��Ҽ����� ������ �����Ͽ� ���� ���� epsilon�� ���Ѵ�.

const float Epsilon = 0.001f;

bool Equals(float lhs, float rhs)
{
	return fabs(lhs - rhs) < Epsilon ? true : false;
}