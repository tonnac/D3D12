#include "ex19.cpp"

int main()
{
	int pp[4][4] = { 0, };
	pp[0][0] = 1;
	pp[0][1] = 2;
	pp[0][2] = 3;
	pp[0][3] = 4;
	pp[1][0] = 5;
	pp[1][1] = 6;
	pp[1][2] = 7;
	pp[1][3] = 8;
	pp[2][0] = 9;
	pp[2][1] = 10;
	pp[2][2] = 11;
	pp[2][3] = 12;
	pp[3][0] = 13;
	pp[3][1] = 14;
	pp[3][2] = 15;
	pp[3][3] = 16;

	//for (int i = 0; i < 4; ++i)
	//{
	//	for (int j = 0; j < 4; ++j)
	//	{
	//		pp[i][j] = (rand() % 10);
	//	}
	//}

	float eee[4][4] = { 0, };

	Matrix::InverseMatrix<int>(pp, eee);

	return 0;
}