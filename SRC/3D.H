/* Simple 3D graphics routines */

/* Written by Bernie Roehl, November 1994 for VictorMaxx Technologies */

/* Redistribute at will, but charge nothing */

typedef struct { float x, y, z; } Vector;

typedef float Matrix[3][3];

typedef struct
	{
	int start;    /* index into vertex array of start of edge */
	int end;      /* index into vertex array of end of edge */
	int color;    /* color to draw this edge in */
	} Edge;

typedef struct
	{
	Vector location;      /* location of object in the world */
	Matrix orientation;   /* orientation matrix for the object */
	int nvertices;        /* number of vertices in the object */
	Vector *vertices;     /* array of vertices */
	int nedges;           /* number of edges in the object */
	Edge *edges;          /* array of edges */
	} Object;

/* Math routines */

void BuildTrigTables(void);  /* called once at start of application */
float sine(int angle);
float cosine(int angle);

/* Vector operations */

void VectorAdd(Vector *result, Vector *a, Vector *b);
void VectorSubtract(Vector *result, Vector *a, Vector *b);
void VectorRotate(Vector *result, Matrix rotmat, Vector *v);

/* Matrix operations */

void MatrixIdentity(Matrix result);
void MatrixMultiply(Matrix result, Matrix a, Matrix b);
void MatrixMakeXrot(Matrix result, int angle);
void MatrixMakeYrot(Matrix result, int angle);
void MatrixMakeZrot(Matrix result, int angle);

/* Low-level screen operations */

int DisplayInitialize(int ntsc);
void DisplayUninitialize(void);
void DisplayClear(int color);
void DisplayLine(int x1, int y1, int x2, int y2, int color);
void DisplayCopyLine(unsigned char far *data, unsigned int line, int length);
void DisplaySetPalette(unsigned char *palette);
void DisplayUpdate(void);
void DisplaySetEye(int eye);  /* eye is zero for left, non-zero for right */
extern int DisplayWidth, DisplayHeight;

/* CTM support */

int CTM_initialize(int port);
void CTM_uninitialize(void);
void CTM_read(int *yaw, int *pitch, int *roll);
void CTM_float_read(float *yaw, float *pitch, float *roll);

/* Renderer */

void Render(Object *object, Matrix eye_matrix, float eye_offset, int x_shift);
void RenderSetZoom(float value);

/* End of 3d.h */
