#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <blossom_common/blossom_common.hpp>
#include <blossom_math/blossom_math.hpp>

#define LOG_TAG "libgl2jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)



#define USE_SCISSOR
#define USE_STENCIL



struct VertexPos
{
	vec3 pos;
};

struct VertexPosNormal
{
	vec3 pos;
	vec3 normal;
};



int screenWidth, screenHeight;

mtx viewTransform;
mtx viewTransformInversed;
mtx viewTransformTransposed;
mtx projTransform;
mtx viewProjTransform;
mtx viewProjTransformTransposed;
float aspect;
float zNear;
float zFar;
float nearPlaneWidth;
float nearPlaneHeight;

char* gbufferVertexShader;
char* gbufferPixelShader;
char* lightVertexShader;
char* lightPixelShader;
char* materialVertexShader;
char* materialPixelShader;
char* unitSphereGeometryData;
char* meshGeometryData;

int unitSphereIndicesNum, meshIndicesNum;
unsigned short *unitSphereIndices, *meshIndices;
int unitSphereVerticesNum, meshVerticesNum;
VertexPos *unitSphereVertices;
VertexPosNormal *meshVertices;

GLuint fbo, depthRenderbuffer, stencilRenderbuffer, gbufferTexture, lightTexture;
GLuint gbufferProgram, lightProgram, materialProgram;
GLuint gbufferAttribPosition, gbufferAttribNormal, lightAttribPosition, materialAttribPosition;
GLuint screenQuadVB, screenQuadIB, unitSphereVB, unitSphereIB, meshVB, meshIB;

float angles[18];



GLuint loadShader(GLenum shaderType, const char* source)
{
	GLuint shader = glCreateShader(shaderType);

	if (shader)
	{
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);
		
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

		if (!compiled)
		{
			GLint infoLogLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

			if (infoLogLength)
			{
				char* infoLog = (char*)malloc(infoLogLength);
				
				if (infoLog)
				{
					glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
					LOGE("Could not compile shader:\n%s\n", infoLog);
					free(infoLog);
				}

				glDeleteShader(shader);
				shader = 0;
			}
		}
	}
	
	return shader;
}



GLuint createProgram(const char* vertexShaderSource, const char* pixelShaderSource)
{
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSource);
	if (!vertexShader)
		return 0;

	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pixelShaderSource);
	if (!pixelShader)
		return 0;

	GLuint program = glCreateProgram();
	
	if (program)
	{
		glAttachShader(program, vertexShader);
		glAttachShader(program, pixelShader);
		glLinkProgram(program);
		
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

		if (linkStatus != GL_TRUE)
		{
			GLint infoLogLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
			
			if (infoLogLength)
			{
				char* infoLog = (char*)malloc(infoLogLength);
				
				if (infoLog)
				{
					glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
					LOGE("Could not link program:\n%s\n", infoLog);
					free(infoLog);
				}
			}

			glDeleteProgram(program);
			program = 0;
		}
	}

	return program;
}



void onSurfaceChanged(int width, int height)
{
	LOGI("Version = %s\n", glGetString(GL_VERSION));
	LOGI("Vendor = %s\n", glGetString(GL_VENDOR));
	LOGI("Renderer = %s\n", glGetString(GL_RENDERER));
	LOGI("Extensions = %s\n", glGetString(GL_EXTENSIONS));

	screenWidth = width;
	screenHeight = height;

	//
	
	glViewport(0, 0, screenWidth, screenHeight);
	
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &depthRenderbuffer);
	glGenRenderbuffers(1, &stencilRenderbuffer);
	glGenTextures(1, &gbufferTexture);
	glGenTextures(1, &lightTexture);

	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, stencilRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, screenWidth, screenHeight);

	glBindTexture(GL_TEXTURE_2D, gbufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, lightTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if (!(gbufferProgram = createProgram(gbufferVertexShader, gbufferPixelShader)))
		LOGE("Could not create gbuffer program.");
	if (!(lightProgram = createProgram(lightVertexShader, lightPixelShader)))
		LOGE("Could not create light program.");
	if (!(materialProgram = createProgram(materialVertexShader, materialPixelShader)))
		LOGE("Could not create material program.");

	gbufferAttribPosition = glGetAttribLocation(gbufferProgram, "attrib_position");
	gbufferAttribNormal = glGetAttribLocation(gbufferProgram, "attrib_normal");
	lightAttribPosition = glGetAttribLocation(lightProgram, "attrib_position");
	materialAttribPosition = glGetAttribLocation(lightProgram, "attrib_position");

	VertexPos screenQuadVertices[4] =
	{
		{ vec3(-1.0f, 1.0f, 0.0f) },
		{ vec3(1.0f, 1.0f, 0.0f) },
		{ vec3(1.0f, -1.0f, 0.0f) },
		{ vec3(-1.0f, -1.0f, 0.0f) },
	};
	unsigned short screenQuadIndices[6] = { 0, 1, 2, 0, 2, 3 };

	glGenBuffers(1, &screenQuadVB);
	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), screenQuadVertices, GL_STATIC_DRAW);
	glGenBuffers(1, &screenQuadIB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenQuadIB);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenQuadIndices), screenQuadIndices, GL_STATIC_DRAW);

	glGenBuffers(1, &unitSphereVB);
	glBindBuffer(GL_ARRAY_BUFFER, unitSphereVB);
	glBufferData(GL_ARRAY_BUFFER, unitSphereVerticesNum * sizeof(VertexPos), unitSphereVertices, GL_STATIC_DRAW);
	glGenBuffers(1, &unitSphereIB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitSphereIB);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, unitSphereIndicesNum * sizeof(unsigned short), unitSphereIndices, GL_STATIC_DRAW);
	
	glGenBuffers(1, &meshVB);
	glBindBuffer(GL_ARRAY_BUFFER, meshVB);
	glBufferData(GL_ARRAY_BUFFER, meshVerticesNum * sizeof(VertexPosNormal), meshVertices, GL_STATIC_DRAW);
	glGenBuffers(1, &meshIB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIB);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndicesNum * sizeof(unsigned short), meshIndices, GL_STATIC_DRAW);
	
	//

	for (int i = 0; i < 18; i++)
		angles[i] = 0.0f;
}



// the following algorithm partially comes from Eric Lengyel's "Mathematics for 3D Game Programming and Computer Graphics" (chapter about shadow volumes)
void setScissorRectForLight(const vec3& pos, float range)
{
	vec4 lp = vec4(pos) * viewTransform; // light's position in view space
	lp.divideByW();
	float r = range;

	// compute tangent planes

	vec3 tl(0.0f), tr(0.0f), tb(0.0f), tt(0.0f); // left, right, bottom, top

	float dx = 4.0f * (r*r*lp.x*lp.x - (lp.x*lp.x + lp.z*lp.z) * (r*r - lp.z*lp.z));
	tl.x = (r*lp.x - sqrtf(dx/4.0f)) / (lp.x*lp.x + lp.z*lp.z);
	tl.z = (r - tl.x*lp.x) / lp.z;
	tr.x = (r*lp.x + sqrtf(dx/4.0f)) / (lp.x*lp.x + lp.z*lp.z);
	tr.z = (r - tr.x*lp.x) / lp.z;

	float dy = 4.0f * (r*r*lp.y*lp.y - (lp.y*lp.y + lp.z*lp.z) * (r*r - lp.z*lp.z));
	tb.y = (r*lp.y - sqrtf(dy/4.0f)) / (lp.y*lp.y + lp.z*lp.z);
	tb.z = (r - tb.y*lp.y) / lp.z;
	tt.y = (r*lp.y + sqrtf(dy/4.0f)) / (lp.y*lp.y + lp.z*lp.z);
	tt.z = (r - tt.y*lp.y) / lp.z;

	tl *= viewTransformInversed;
	tr *= viewTransformInversed;
	tb *= viewTransformInversed;
	tt *= viewTransformInversed;

	tl.setLength(r);
	tr.setLength(r);
	tb.setLength(r);
	tt.setLength(r);

	vec4 corners[4]; // eye-sphere points of tangency

	corners[0] = vec4(pos - tl);
	corners[1] = vec4(pos - tr);
	corners[2] = vec4(pos - tb);
	corners[3] = vec4(pos - tt);

	for (int i = 0; i < 4; i++)
	{
		corners[i] *= viewProjTransform;
		corners[i].divideByW(); // this operation will cause artefacts if any of the light's corners is behind the near plane
	}

	// right now corners are in clip-space

	vec4 min, max;

	min = vec4(corners[0].x, corners[0].y, 0.0f);
	max = vec4(corners[0].x, corners[0].y, 0.0f);

	for (int i = 1; i < 4; i++)
	{
		if (corners[i].x < min.x)
			min.x = corners[i].x;
		if (corners[i].y < min.y)
			min.y = corners[i].y;

		if (corners[i].x > max.x)
			max.x = corners[i].x;
		if (corners[i].y > max.y)
			max.y = corners[i].y;
	}

	vec2 min2(min.x, min.y);
	vec2 max2(max.x, max.y);
	min2 = 0.5f*min2 + vec2(0.5f, 0.5f);
	max2 = 0.5f*max2 + vec2(0.5f, 0.5f);
	min2 *= vec2(screenWidth, screenHeight);
	max2 *= vec2(screenWidth, screenHeight);

	glScissor(min2.x, min2.y, max2.x-min2.x, max2.y-min2.y);
}



void renderLight(const vec3& pos, float range, const vec3& color)
{
	mtx temp;
	#ifndef USE_STENCIL
		temp = mtx::identity(); // we use screen-aligned quad
	#else
		temp = mtx::scale(range, range, range) * mtx::translate(pos) * viewProjTransform; // we use 3D sphere	
	#endif
	temp.transpose();
	glUniformMatrix4fv(glGetUniformLocation(lightProgram, "viewProjTransform"), 1, false, &temp._[0][0]);

	vec4 lightPos = vec4(pos) * viewTransform;
	vec4 lightColor(color);
	glUniform4f(glGetUniformLocation(lightProgram, "lightPosAndRange"), lightPos.x, lightPos.y, lightPos.z, 1.0f/(range*range));
	glUniform4f(glGetUniformLocation(lightProgram, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);

	#ifdef USE_SCISSOR
		setScissorRectForLight(pos, range);
	#endif

	#ifndef USE_STENCIL
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	#else
		glClear(GL_STENCIL_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, 0, 0x1);
		glStencilFuncSeparate(GL_BACK, GL_ALWAYS, 0, 0x1);
		glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
		glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
		glDrawElements(GL_TRIANGLES, unitSphereIndicesNum, GL_UNSIGNED_SHORT, 0);

		glEnable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilFunc(GL_NOTEQUAL, 0, 0x1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glDrawElements(GL_TRIANGLES, unitSphereIndicesNum, GL_UNSIGNED_SHORT, 0);
	#endif
}



void onDrawFrame()
{
	aspect = (float)screenWidth / (float)screenHeight;
	zNear = 1.0f;
	zFar = 250.0f;

	viewTransform = mtx::lookAtLH(vec3(0.0f, 12.0f, 20.0f), vec3(0.0f, -15.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	viewTransformInversed = viewTransform.getInversed();
	viewTransformTransposed = viewTransform.getTransposed();
	projTransform = mtx::perspectiveFovLH(PI/3.0f, aspect, zNear, zFar);
	viewProjTransform = viewTransform * projTransform;
	viewProjTransformTransposed = viewProjTransform.getTransposed();

	vec4 leftTopCorner = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	vec4 rightBottomCorner = vec4(1.0f, -1.0f, 0.0f, 1.0f);
	leftTopCorner *= projTransform.getInversed();
	rightBottomCorner *= projTransform.getInversed();
	leftTopCorner.divideByW();
	rightBottomCorner.divideByW();
	nearPlaneWidth = rightBottomCorner.x - leftTopCorner.x;
	nearPlaneHeight = leftTopCorner.y - rightBottomCorner.y;



	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepthf(1.0f);
	glClearStencil(0);



	// g-buffer pass

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	glViewport(0, 0, screenWidth, screenHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbufferTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRenderbuffer);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, meshVB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIB);
	glDisableVertexAttribArray(lightAttribPosition);
	glDisableVertexAttribArray(materialAttribPosition);
	glEnableVertexAttribArray(gbufferAttribPosition);
	glEnableVertexAttribArray(gbufferAttribNormal);
	glVertexAttribPointer(gbufferAttribPosition, 3, GL_FLOAT, GL_TRUE, sizeof(VertexPosNormal), 0);
	glVertexAttribPointer(gbufferAttribNormal, 3, GL_FLOAT, GL_TRUE, sizeof(VertexPosNormal), (char*)0 + sizeof(vec3));

	glUseProgram(gbufferProgram);
	
	glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "viewTransform"), 1, false, &viewTransformTransposed._[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "viewProjTransform"), 1, false, &viewProjTransformTransposed._[0][0]);

	glDrawElements(GL_TRIANGLES, meshIndicesNum, GL_UNSIGNED_SHORT, 0);



	// lights pass

	#ifndef USE_STENCIL
		glDisable(GL_DEPTH_TEST);
	#else
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_STENCIL_TEST);		
	#endif
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRenderbuffer);

	glClear(GL_COLOR_BUFFER_BIT);

	#ifndef USE_STENCIL	
		glBindBuffer(GL_ARRAY_BUFFER, screenQuadVB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenQuadIB);		
	#else
		glBindBuffer(GL_ARRAY_BUFFER, unitSphereVB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitSphereIB);
	#endif
	glDisableVertexAttribArray(gbufferAttribPosition);
	glDisableVertexAttribArray(gbufferAttribNormal);
	glDisableVertexAttribArray(materialAttribPosition);
	glEnableVertexAttribArray(lightAttribPosition);
	glVertexAttribPointer(lightAttribPosition, 3, GL_FLOAT, GL_TRUE, sizeof(VertexPos), 0);

	glUseProgram(lightProgram);
	
	glUniform2f(glGetUniformLocation(lightProgram, "nearPlaneSize"), nearPlaneWidth/zNear, nearPlaneHeight/zNear);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbufferTexture);
	glUniform1i(glGetUniformLocation(lightProgram, "gbufferTexture"), 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	#ifdef USE_SCISSOR
		glEnable(GL_SCISSOR_TEST);
	#endif

	static float angle = 0.0f;
	angle += 0.01f;
	for (int j = 0; j < 3; j++)
	{
		vec3 color = vec3(1.0f, 0.0f, 0.0f);
		if (j == 1)
			color = vec3(0.0f, 1.0f, 0.0f);
		else if (j == 2)
			color = vec3(0.0f, 0.0f, 1.0f);

		for (int i = 0; i < 6; i++)
		{
			float speed = 0.05f*j + (float)i/100.0f;
			angles[3*j + i] += speed;
		//	angles[3*j + i] = (float)(3*j + i);

			float x = 0.0f + 5.0f*cosf(angles[3*j + i]);
			float z = 10.0f + 5.0f*sinf(angles[3*j + i]);

			renderLight(vec3(x, 1.5f, z), 3.0f, color);
		}
	}
	
	#ifdef USE_SCISSOR
		glDisable(GL_SCISSOR_TEST);
	#endif

	glDisable(GL_BLEND);



	// material pass

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, meshVB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIB);
	glDisableVertexAttribArray(gbufferAttribPosition);
	glDisableVertexAttribArray(gbufferAttribNormal);
	glDisableVertexAttribArray(lightAttribPosition);
	glEnableVertexAttribArray(materialAttribPosition);
	glVertexAttribPointer(materialAttribPosition, 3, GL_FLOAT, GL_TRUE, sizeof(VertexPosNormal), 0);

	glUseProgram(materialProgram);
	
	glUniformMatrix4fv(glGetUniformLocation(materialProgram, "viewProjTransform"), 1, false, &viewProjTransformTransposed._[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lightTexture);
	glUniform1i(glGetUniformLocation(materialProgram, "lightTexture"), 0);

	glDrawElements(GL_TRIANGLES, meshIndicesNum, GL_UNSIGNED_SHORT, 0);
}



extern "C"
{
	JNIEXPORT void JNICALL Java_maxest_samples_DeferredLightingActivity_passInputData(
		JNIEnv * env, jobject obj,
		jstring gbufferVertexShaderString, jstring gbufferPixelShaderString,
		jstring lightVertexShaderString, jstring lightPixelShaderString,
		jstring materialVertexShaderString, jstring materialPixelShaderString,
		jstring unitSphereGeometryDataString, jstring meshGeometryDataString);
	JNIEXPORT void JNICALL Java_maxest_samples_DeferredLightingActivity_rendererOnSurfaceChanged(JNIEnv * env, jobject obj, jint width, jint height);
	JNIEXPORT void JNICALL Java_maxest_samples_DeferredLightingActivity_rendererOnDrawFrame(JNIEnv * env, jobject obj);
};



JNIEXPORT void JNICALL Java_maxest_samples_DeferredLightingActivity_passInputData(
		JNIEnv * env, jobject obj,
		jstring gbufferVertexShaderString, jstring gbufferPixelShaderString,
		jstring lightVertexShaderString, jstring lightPixelShaderString,
		jstring materialVertexShaderString, jstring materialPixelShaderString,
		jstring unitSphereGeometryDataString, jstring meshGeometryDataString)
{
	const char* t1 = env->GetStringUTFChars(gbufferVertexShaderString, NULL);
	const char* t2 = env->GetStringUTFChars(gbufferPixelShaderString, NULL);
	const char* t3 = env->GetStringUTFChars(lightVertexShaderString, NULL);
	const char* t4 = env->GetStringUTFChars(lightPixelShaderString, NULL);
	const char* t5 = env->GetStringUTFChars(materialVertexShaderString, NULL);
	const char* t6 = env->GetStringUTFChars(materialPixelShaderString, NULL);
	const char* t7 = env->GetStringUTFChars(unitSphereGeometryDataString, NULL);
	const char* t8 = env->GetStringUTFChars(meshGeometryDataString, NULL);

	gbufferVertexShader = new char[strlen(t1) + 1];
	gbufferPixelShader = new char[strlen(t2) + 1];
	lightVertexShader = new char[strlen(t3) + 1];
	lightPixelShader = new char[strlen(t4) + 1];
	materialVertexShader = new char[strlen(t5) + 1];
	materialPixelShader = new char[strlen(t6) + 1];
	unitSphereGeometryData = new char[strlen(t7) + 1];
	meshGeometryData = new char[strlen(t8) + 1];

	strcpy(gbufferVertexShader, t1);
	strcpy(gbufferPixelShader, t2);
	strcpy(lightVertexShader, t3);
	strcpy(lightPixelShader, t4);
	strcpy(materialVertexShader, t5);
	strcpy(materialPixelShader, t6);
	strcpy(unitSphereGeometryData, t7);
	strcpy(meshGeometryData, t8);

	gbufferVertexShader[strlen(t1)] = '\0';
	gbufferPixelShader[strlen(t2)] = '\0';
	lightVertexShader[strlen(t3)] = '\0';
	lightPixelShader[strlen(t4)] = '\0';
	materialVertexShader[strlen(t5)] = '\0';
	materialPixelShader[strlen(t6)] = '\0';
	unitSphereGeometryData[strlen(t7)] = '\0';
	meshGeometryData[strlen(t8)] = '\0';

	{
		stringstream ss(unitSphereGeometryData);

		ss >> unitSphereIndicesNum;
		unitSphereIndices = new unsigned short[unitSphereIndicesNum];
		for (int i = 0; i < unitSphereIndicesNum; i += 3)
		{
			ss >> unitSphereIndices[i + 0];
			ss >> unitSphereIndices[i + 1];
			ss >> unitSphereIndices[i + 2];
		}

		ss >> unitSphereVerticesNum;
		unitSphereVertices = new VertexPos[unitSphereVerticesNum];
		for (int i = 0; i < unitSphereVerticesNum; i++)
		{
			float dummy;

			ss >> unitSphereVertices[i].pos.x;
			ss >> unitSphereVertices[i].pos.y;
			ss >> unitSphereVertices[i].pos.z;
			ss >> dummy;
			ss >> dummy;
			ss >> dummy;
		}
	}
	
	{
		stringstream ss(meshGeometryData);

		ss >> meshIndicesNum;
		meshIndices = new unsigned short[meshIndicesNum];
		for (int i = 0; i < meshIndicesNum; i += 3)
		{
			ss >> meshIndices[i + 0];
			ss >> meshIndices[i + 1];
			ss >> meshIndices[i + 2];
		}

		ss >> meshVerticesNum;
		meshVertices = new VertexPosNormal[meshVerticesNum];
		for (int i = 0; i < meshVerticesNum; i++)
		{
			ss >> meshVertices[i].pos.x;
			ss >> meshVertices[i].pos.y;
			ss >> meshVertices[i].pos.z;
			ss >> meshVertices[i].normal.x;
			ss >> meshVertices[i].normal.y;
			ss >> meshVertices[i].normal.z;
		}
	}

	delete[] t1;
	delete[] t2;
	delete[] t3;
	delete[] t4;
	delete[] t5;
	delete[] t6;
	delete[] t7;
	delete[] t8;
}



JNIEXPORT void JNICALL Java_maxest_samples_DeferredLightingActivity_rendererOnSurfaceChanged(JNIEnv * env, jobject obj,  jint width, jint height)
{
	onSurfaceChanged(width, height);
}



JNIEXPORT void JNICALL Java_maxest_samples_DeferredLightingActivity_rendererOnDrawFrame(JNIEnv * env, jobject obj)
{
	onDrawFrame();
}
