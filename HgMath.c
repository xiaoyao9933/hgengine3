#include <stdint.h>
#include <HgMath.h>
#include <stdio.h>

#include <math.h>
#include <memory.h>

void MatrixMultiply4f(const float* in1, const float* in2, float* outa)
{
	float* r = outa;

	r[0] = in1[0] * in2[0] + in1[1] * in2[4] +
		in1[2] * in2[8] + in1[3] * in2[12];
	r[1] = in1[0] * in2[1] + in1[1] * in2[5] +
		in1[2] * in2[9] + in1[3] * in2[13];
	r[2] = in1[0] * in2[2] + in1[1] * in2[6] +
		in1[2] * in2[10] + in1[3] * in2[14];
	r[3] = in1[0] * in2[3] + in1[1] * in2[7] +
		in1[2] * in2[11] + in1[3] * in2[15];

	r[4] = in1[4] * in2[0] + in1[5] * in2[4] +
		in1[6] * in2[8] + in1[7] * in2[12];
	r[5] = in1[4] * in2[1] + in1[5] * in2[5] +
		in1[6] * in2[9] + in1[7] * in2[13];
	r[6] = in1[4] * in2[2] + in1[5] * in2[6] +
		in1[6] * in2[10] + in1[7] * in2[14];
	r[7] = in1[4] * in2[3] + in1[5] * in2[7] +
		in1[6] * in2[11] + in1[7] * in2[15];

	r[8] = in1[8] * in2[0] + in1[9] * in2[4] +
		in1[10] * in2[8] + in1[11] * in2[12];
	r[9] = in1[8] * in2[1] + in1[9] * in2[5] +
		in1[10] * in2[9] + in1[11] * in2[13];
	r[10] = in1[8] * in2[2] + in1[9] * in2[6] +
		in1[10] * in2[10] + in1[11] * in2[14];
	r[11] = in1[8] * in2[3] + in1[9] * in2[7] +
		in1[10] * in2[11] + in1[11] * in2[15];

	r[12] = in1[12] * in2[0] + in1[13] * in2[4] +
		in1[14] * in2[8] + in1[15] * in2[12];
	r[13] = in1[12] * in2[1] + in1[13] * in2[5] +
		in1[14] * in2[9] + in1[15] * in2[13];
	r[14] = in1[12] * in2[2] + in1[13] * in2[6] +
		in1[14] * in2[10] + in1[15] * in2[14];
	r[15] = in1[12] * in2[3] + in1[13] * in2[7] +
		in1[14] * in2[11] + in1[15] * in2[15];
}

void print_matrix(const float* m) {
	uint8_t i;
	for (i = 0; i < 4; ++i) {
		printf("%f %f %f %f\n", m[i*4], m[i*4 + 1], m[i*4 + 2], m[i*4 + 3]);
	}
}


void Perspective(
	double fov,
	const double aspect,
	const double znear,
	const double zfar, float* M)
{
	double top = tan(fov*0.5 * RADIANS) * znear;
	double bottom = -top;
	double right = aspect*top;
	double left = -right;

	M[0] = (2 * znear) / (right - left);
	M[2] = (right + left) / (right - left);
	M[6] = (top + bottom) / (top - bottom);
	M[5] = (2 * znear) / (top - bottom);
	M[10] = (zfar + znear) / (zfar - znear);
	M[11] = (2 * zfar*znear) / (zfar - znear);
	M[14] = -1.0f;
}

void Perspective2(
	double fov,
	const double aspect,
	const double znear,
	const double zfar, float* M)
{
	fov *= RADIANS;

	double f = 1.0 / tan(fov*0.5);
	memset(M, 0, 16 * sizeof* M);


	M[0] = f / aspect;
	M[5] = f;
	M[10] = (zfar + znear) / (znear - zfar);
	M[11] = (2 * zfar*znear) / (znear - zfar);
	M[14] = -1.0f;
}

/*
// set the OpenGL perspective projection matrix
void glFrustum(
const float &bottom, const float &top, const float &left, const float &right,
const float &zNear, const float &zFar,
float *M)
{
//Should go in projection matrix
float near2 = 2 * zNear;
float rml = right - left;
float tmb = top - bottom;
float fmn = zFar - zNear;

float A = (right + left) / rml;
float B = (top + bottom) / tmb;
float C = -(zFar + zNear) / fmn;
float D = -(near2*zFar*zNear) / fmn;

memset(M, 0, 16 * sizeof* M);

//row major
M[0] = near2 / rml;
M[2] = A;
M[5] = near2 / tmb;
M[6] = B;
M[10] = C;
M[11] = D;
M[14] = -1;
}
*/