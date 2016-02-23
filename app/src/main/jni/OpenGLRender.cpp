#include <GLES2/gl2.h>
#include <stdlib.h>
#include <stdio.h>

#include "OpenGLRender.h"
#include "ImageFilter.h"

namespace KugouPlayer
{
	void RGBA2BGR(uint8 * des, uint8 * src, int width, int height)
	{
		for (int y = 0; y < height; y++)
		{
			uint8 * p0 = des + y * WIDTHBYTES(24 * width);
			uint8 * p1 = src + y * WIDTHBYTES(32 * width);

			for (int x = 0; x < width; x++)
			{
				*(p0 + 0) = *(p1 + 2);
				*(p0 + 1) = *(p1 + 1);
				*(p0 + 2) = *(p1 + 0);

				p0 += 3;
				p1 += 4;
			}
		}
	}

	void BGRA2RGBA(uint8 * des, uint8 * src, int width, int height)//24λbmp�����ǰ�BGR��˳����,BΪ���ֽ�,RΪ���ֽ�
	{
		for (int y = 0; y < height; y++)
		{
			uint8 * p0 = des + y * WIDTHBYTES(32 * width);
			uint8 * p1 = src + y * WIDTHBYTES(32 * width);

			for (int x = 0; x < width; x++)
			{
				*(p0 + 0) = *(p1 + 2);
				*(p0 + 1) = *(p1 + 1);
				*(p0 + 2) = *(p1 + 0);
				*(p0 + 3) = 0xff;

				p0 += 4;
				p1 += 4;
			}
		}
	}

	void RGBA2BGRA(uint8 * des, uint8 * src, int width, int height)
	{
		for (int y = 0; y < height; y++)
		{
			uint8 * p0 = des + y * WIDTHBYTES(32 * width);
			uint8 * p1 = src + y * WIDTHBYTES(32 * width);

			for (int x = 0; x < width; x++)
			{
				*(p0 + 0) = *(p1 + 2);
				*(p0 + 1) = *(p1 + 1);
				*(p0 + 2) = *(p1 + 0);
				*(p0 + 3) = 0xff;

				p0 += 4;
				p1 += 4;
			}
		}
	}

	/*
	 *��ɫ������ɫ����
	��ɫ������һ����C���ԣ�������C����һ��֧��˫���ȸ�����(double)���ֽ���(byte)��������(short)��������(long)������ȡ����C�е�������(union)��ö������(enum)���޷�����(unsigned)�Լ�λ��������ԡ�
	��ɫ������������ڽ���ԭ�����������Լ������������ͣ��磺������(float)��������(bool)������(int)��������(matrix)�Լ�������(vec2��vec3��)�ȡ�������˵����Щ�������Ϳ��Է�Ϊ���������������󡢲��������ṹ���Լ�����ȡ�
	shader֧�������������ͣ�
	Float  bool  int
	vec2           ������2��������������     vec3          ������3��������������     vec4         ������4��������������
	ivec2          ������2������������         ivec3       ������3������������      ivec4       ������4������������
	bvec2          ������2��������������    bvec3        ������3��������������   bvec4       ������4��������������
	mat2    2*2ά����                           mat3   3*3ά����                          mat4    4*4ά����
    sampler1D   1D������ɫ��          sampler2D    2D������ɫ��
	sampler3D   3D������ɫ��

	OpenGL ES2.0��shader���������ı���һ������������uniform, attribute, varying��
	1.uniform����
	uniform�������ⲿapplication���򴫵ݸ���vertex��fragment��shader�ı������������ applicationͨ������glUniform**����������ֵ�ġ��ڣ�vertex��fragment��shader�����ڲ���uniform���� ������C��������ĳ�����const ���������ܱ�shader�����޸ġ���shaderֻ���ã����ܸģ�
	���uniform������vertex��fragment����֮��������ʽ��ȫһ��������������vertex��fragment����ʹ�á����൱��һ����vertex��fragment shader�����ȫ�ֱ�����
	uniform����һ��������ʾ���任���󣬲��ʣ����ղ�������ɫ����Ϣ��
	2.attribute����
	attribute������ֻ����vertex shader��ʹ�õı���������������fragment shader������attribute������Ҳ���ܱ�fragment shader��ʹ�ã�
	һ����attribute��������ʾһЩ��������ݣ��磺�������꣬���ߣ��������꣬������ɫ�ȡ�
	��application�У�һ���ú���glBindAttribLocation��������ÿ��attribute������λ�ã�Ȼ���ú���glVertexAttribPointer����Ϊÿ��attribute������ֵ��
	3.varying����
	varying������vertex��fragment shader֮�������ݴ����õġ�һ��vertex shader�޸�varying������ֵ��Ȼ��fragment shaderʹ�ø�varying������ֵ�����varying������vertex��fragment shader����֮�������������һ�µġ�
	application����ʹ�ô˱�����
	 */
	const char* OpenGLRender::VERTEX_SHADER_STRING =
		"attribute vec4 position;"
		"attribute vec2 texcoord;"
		"varying vec2 v_texcoord;"
		"void main() {"
		"   gl_Position = position;"
		"	v_texcoord = texcoord;"
		"}";

#if 0
	const char* OpenGLRender::YUV_FRAGMENT_SHADER_STRING =
		"varying lowp vec2 v_texcoord;"
		"uniform sampler2D s_texture;"
		"void main(void) {"
		"	gl_FragColor = texture2D(s_texture, v_texcoord);"
		"}";
#else
	const char* OpenGLRender::YUV_FRAGMENT_SHADER_STRING =
		"varying lowp vec2 v_texcoord;\n"
		"uniform sampler2D s_texture;\n"
		"uniform lowp vec2 TexSize;\n"
		"uniform lowp int enablegaussfilter;\n"
		"mediump float kernal[21];\n"
		"mediump float softlight(mediump float a, mediump float b)\n"
		"{\n"
		"  if(b <= 0.5)\n"
		"  {\n"
		"    return a * b / 0.5 + (a / 1.0) * (a / 1.0) * (1.0 - 2.0 * b);\n"
		"  }\n"
		"  else\n"
		"  {\n"
		"    return a * (1.0 - b) / 0.5 + sqrt(a / 1.0) * (2.0 * b - 1.0);\n"
		"  }\n"
		"}\n"
		"mediump vec4 softlight(mediump vec4 a, mediump vec4 b)\n"
		"{\n"
		"  return vec4(softlight(a.x, b.x), softlight(a.y, b.y), softlight(a.z, b.z), 1.0);\n"
		"}\n"
		"mediump vec4 gauss_filter(mediump float _kernal[21], sampler2D _image, lowp vec2 _uv, lowp vec2 _texSize)\n"
		"{\n"
		"  mediump vec4 oc = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"  mediump float dx = 1.0 / _texSize.x;\n"
		"  mediump float dy = 1.0 / _texSize.y;\n"
		"  for (int n = 0, i = -10; i <= 10; i++, n++)\n"
		"  {\n"
		"    lowp vec2 _uv_new = vec2(_uv.x + float( i ) * dx, _uv.y);\n"
		"    oc += texture2D(_image, _uv_new) * _kernal[n];\n"
		"  }\n"
		"  mediump vec4 nc = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"  for (int n = 0, i = -10; i <= 10; i++, n++)\n"
		"  {\n"
		"    lowp vec2 _uv_new = vec2(_uv.x, _uv.y + float( i ) * dy);\n"
		"    nc += texture2D(_image, _uv_new) * _kernal[n];\n"
		"  }\n"
		"  return (oc + nc) / vec4(2.0);\n"
		"}\n"
		"mediump vec4 alpha_blend(mediump vec4 a, mediump vec4 b)\n"
		"{\n"
		"  return a * vec4(1.0 - b.a) + b * vec4(b.a);\n"
		"}\n"
		"void main(void)\n"
		"{\n"
		"  kernal[0] = float(0.001332);kernal[1] = float(0.003131);kernal[2] = float(0.006729);\n"
		"  kernal[3] = float(0.013216);kernal[4] = float(0.023722);kernal[5] = float(0.038916);\n"
		"  kernal[6] = float(0.058347);kernal[7] = float(0.079951);kernal[8] = float(0.100124);\n"
		"  kernal[9] = float(0.114596);kernal[10] = float(0.119871);kernal[11] = float(0.114596);\n"
		"  kernal[12] = float(0.100124);kernal[13] = float(0.079951);kernal[14] = float(0.058347);\n"
		"  kernal[15] = float(0.038916);kernal[16] = float(0.023722);kernal[17] = float(0.013216);\n"
		"  kernal[18] = float(0.006729);kernal[19] = float(0.003131);kernal[20] = float(0.001332);\n"
		"  mediump vec4 a = texture2D(s_texture, v_texcoord);\n"
		"  if( enablegaussfilter == 0 )\n"
		"  {\n"
		"    gl_FragColor = a;\n"
		"  }\n"
		"  else\n"
		"  {\n"
		"    mediump vec4 b = gauss_filter(kernal, s_texture, v_texcoord, TexSize);\n"
		"    mediump vec4 c = softlight(a, b);\n"
		"    gl_FragColor = c;\n"
		"  }"
		"}\n";
#endif

	const float OpenGLRender::squardVertices[ 8 ] =
		{
			-1.0f, -1.0f,
			1.0f, -1.0f,
			-1.0f,  1.0f,
			1.0f,  1.0f
		};

	const float OpenGLRender::TEXTURE_ROTATED[ 4 ][ 8 ] =
		{
			// normal
			{
				0.0f, 1.0f,
				1.0f, 1.0f,
				0.0f, 0.0f,
				1.0f, 0.0f,
			},
			// rotate 90
			{
				1.0f, 1.0f,
				1.0f, 0.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,
			},
			// rotate 180
			{
				1.0f, 0.0f,
				0.0f, 0.0f,
				1.0f, 1.0f,
				0.0f, 1.0f,
			},
			// rotate 270
			{
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 0.0f,
				1.0f, 1.0f,
			}
		};

	/*
	 * OpenGLRender��Ĺ��캯���г�ʼ�����Ա����
	 */
	OpenGLRender::OpenGLRender()
		: mGaussFilterFlag( false ),
		  mGaussFilterDrawFlag( true ),
		  program( 0 ),
		  textureId( 0 ),
		  texture( 0 ),
		  texturesize( 0 ),
		  enablegaussfilter( 0 ),
		  vertexShader( 0 ),
		  fragmentShader( 0 ),
		  x( 0 ),
		  y( 0 ),
		  width( 0 ),
		  height( 0 )
	{
		//��������û�о����Ƕ���ת�Ķ�������ֵ������coordVertices��
		memcpy( coordVertices, TEXTURE_ROTATED[ 0 ], sizeof( coordVertices ) );

		int err = _LoadShader();//������ɫ��
		if( err < 0 )
		{
			LOGE( "openGL load shaders failed! err:%d\n", err );
		}
		else
		{
			LOGE( "openGL load shaders success!!!\n" );
		}

		/*
		 * ʹ��glUseProgram()��OpenGL��Ⱦ�ܵ��л�����ɫ��ģʽ����ʹ�øղ����õģ���ɫ���������Ȼ�󣬲ſ����ύ���㡣
		 */
		glUseProgram( program );

		/*
		 * glGenTextures�������������������n�������������������Ƽ��ϲ�����һ����������������
		 * glGenTextures��������������Ҫ�������������������ģ����������OpenGL������Ҫ5��������������û���õ��������ﷵ��5�����㡣
		 * ����ĵ�������Ҫ1���������,Ȼ�������������ֵ����textureId��
		 */
		glGenTextures( 1, &textureId );
	}

	OpenGLRender::~OpenGLRender()
	{
		Cleanup();
	}

	void OpenGLRender::setDisplayArea( int x, int y, int width, int height )
	{
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
	}

	void OpenGLRender::setRotation( int degrees, int flipHorizontal, int flipVertical )
	{
		/*
		 * ������ת�������ò�ͬ����ת������ֵ
		 */
		float rotatedTex[ 8 ];
		if( degrees == 90 )
		{
			memcpy( rotatedTex, TEXTURE_ROTATED[ 1 ], sizeof( rotatedTex ) );
		}
		else if( degrees == 180 )
		{
			memcpy( rotatedTex, TEXTURE_ROTATED[ 2 ], sizeof( rotatedTex ) );
		}
		else if( degrees == 270 )
		{
			memcpy( rotatedTex, TEXTURE_ROTATED[ 3 ], sizeof( rotatedTex ) );
		}
		else
		{
			memcpy( rotatedTex, TEXTURE_ROTATED[ 0 ], sizeof( rotatedTex ) );
		}

		if ( flipHorizontal )
		{
			rotatedTex[ 0 ] = flip( rotatedTex[ 0 ] );
			rotatedTex[ 2 ] = flip( rotatedTex[ 2 ] );
			rotatedTex[ 4 ] = flip( rotatedTex[ 4 ] );
			rotatedTex[ 6 ] = flip( rotatedTex[ 6 ] );
		}
		if ( flipVertical )
		{
			rotatedTex[ 1 ] = flip( rotatedTex[ 1 ] );
			rotatedTex[ 3 ] = flip( rotatedTex[ 3 ] );
			rotatedTex[ 5 ] = flip( rotatedTex[ 5 ] );
			rotatedTex[ 7 ] = flip( rotatedTex[ 7 ] );
		}

		//��Ӧ����ת�Ƕȸ�����ֵ����Ⱦ������render�лᱻʵ��ʹ�õ�
		memcpy( coordVertices, rotatedTex, sizeof( coordVertices ) );

		LOGE( "setRotation degrees:%d, flip:%d %d\n", degrees, flipHorizontal, flipVertical );
	}

	float OpenGLRender::flip( float i )
	{
		return 1.0f - i;
	}

	void OpenGLRender::render( unsigned char* buffer, int widthTexture, int heightTexture )
	{
		//unsigned char* rgbbuffer = NULL;
		//int lineBytes = 0;
		//int imageSize = 0;

		//lineBytes =  WIDTHBYTES(widthTexture * 32);
		//imageSize =  lineBytes * heightTexture;

		_RegenTextures();

		/*
		 * glViewport��Ҫ��������Ĺ��ܡ���������Ӿ����ȡ��ͼ���������ĸߺͿ���ʾ����Ļ�ϡ�
		 */
		glViewport( this->x, this->y, this->width, this->height );
		//ָ����ɫ������������ֵ
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		//���뻺���־λ��������Ҫ����Ļ���
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//LOGE("x=%d y=%d width=%d height=%d widthTexture=%d heightTexture=%d\r\n", x, y, this->width, this->height, widthTexture, heightTexture);

		if( buffer != NULL )
		{
			//rgbbuffer = new unsigned char[imageSize];

			//��֮ǰ���ɵ������������ֵ
			glBindTexture( GL_TEXTURE_2D, textureId );


			//BGRA2RGBA(rgbbuffer,buffer, widthTexture, heightTexture);


			/*
			        ������˺���glTexParameteri()
			         ͼ�������ͼ��ռ�ӳ�䵽֡����ͼ��ռ�(ӳ����Ҫ���¹�������ͼ��,�����ͻ����Ӧ�õ�������ϵ�ͼ��ʧ��),��ʱ�Ϳ���glTexParmeteri()������ȷ����ΰ���������ӳ�������.
			   GL_TEXTURE_WRAP_S: S�����ϵ���ͼģʽ  GL_TEXTURE_WRAP_T:T�����ϵ���ͼģʽ.
			   GL_TEXTURE_MAG_FILTER: �Ŵ����   GL_TEXTURE_MIN_FILTER: ��С����
			   GL_LINEAR: ���Թ���, ʹ�þ��뵱ǰ��Ⱦ�������������4�����ؼ�Ȩƽ��ֵ.
			   GL_NEAREST:����ӽ���ǰ����εĽ����ȵ������㼶��ͼ���в���,Ȼ����������ֵ�������Բ�ֵ.
			 */
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			/*
			 *ʹ��glVertexAttribPointer��ʾ��Ӧ�ó�����Ϊ������ɫ�������е�attribute���͵ı�����ֵ
			 *��һ������index:ָ��Ҫ�޸ĵĶ������Ե�����ֵ  �ڶ���������ָ��ÿ���������Ե��������(һ�������ɶ��ٸ�ֵ���)������Ϊ1��2��3����4����ʼֵΪ4������ά����position����3����x,y,z����ɣ�����ɫ��4����r,g,b,a����
			 *����������type:ָ��������ÿ��������������͡����õķ��ų�����GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,GL_UNSIGNED_SHORT, GL_FIXED, �� GL_FLOAT����ʼֵΪGL_FLOAT��
			 *���ĸ�����normalized:ָ����������ʱ���̶�������ֵ�Ƿ�Ӧ�ñ���һ����GL_TRUE������ֱ��ת��Ϊ�̶���ֵ��GL_FALSE��
			    ���������:stride ָ��������������֮���ƫ���������Ϊ0����ô�������Իᱻ���Ϊ�������ǽ���������һ��ġ���ʼֵΪ0��
			   ����������:pointer ָ��һ��ָ�룬ָ�������е�һ���������Եĵ�һ���������ʼֵΪ0��
			 */
			glVertexAttribPointer( ATTRIBUTE_VERTEX, 2, GL_FLOAT, 0, 0, squardVertices );
			/*
			 * Ҫ���û��߽��ö����������飬����glEnableVertexAttribArray��glDisableVertexAttribArray�������index��������ã���ô��glDrawArrays����glDrawElements������ʱ��������������ᱻʹ�á�
			 */
			glEnableVertexAttribArray( ATTRIBUTE_VERTEX );

			glVertexAttribPointer( ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, 0, 0, coordVertices );
			glEnableVertexAttribArray( ATTRIBUTE_TEXCOORD );

			glActiveTexture( GL_TEXTURE0 );//��������
			glBindTexture( GL_TEXTURE_2D, textureId );

			//Ϊ֮ǰ�󶨵�Ƭ����ɫ�����Ե�uniform���͵ı������и�ֵ
			glUniform1i( texture, 0 );//ֻ�ܴ�0,����ֵ���Ǻ���

			glUniform2f( texturesize, ( GLfloat )widthTexture, ( GLfloat )heightTexture );

			if( mGaussFilterDrawFlag != mGaussFilterFlag )
			{
				mGaussFilterDrawFlag = mGaussFilterFlag;
				glUniform1i( enablegaussfilter, mGaussFilterDrawFlag ? 1 : 0 );
			}

			/*
			 ����ָ���Ĳ���������һ��2D����(Texture)
			 ��һ������ָ��Ŀ���������ֵ������GL_TEXTURE_2D,�ڶ�������levelִ��ϸ�ڼ���0���������ͼ�񼶱�,n��ʾ��N����ͼϸ������
			 ����������internalformat ָ�������е���ɫ��������ȡֵ�ͺ����formatȡֵ������ͬ����ѡ��ֵ��GL_ALPHA,GL_RGB,GL_RGBA,GL_LUMINANCE, GL_LUMINANCE_ALPHA �ȼ��֡�
			���ĸ�����width ָ������ͼ��Ŀ�ȣ����������height ָ������ͼ��ĸ߶ȣ�����������border ָ���߿�Ŀ�ȣ�����Ϊ0.���߸�����format �������ݵ���ɫ��ʽ�������internalformattȡֵ������ͬ��
			�ڰ˸�����type ָ���������ݵ��������͡�����ʹ�õ�ֵ��GL_UNSIGNED_BYTE,GL_UNSIGNED_SHORT_5_6_5,GL_UNSIGNED_SHORT_4_4_4_4,GL_UNSIGNED_SHORT_5_5_5_1��
			�ھŸ�����pixels ָ���ڴ���ָ��ͼ�����ݵ�ָ��
			�ڵ��øú���֮ǰ���������glBindTexture(GL_TEXTURE_2D, mTextureID );��ָ��Ҫ����������ID
			*/
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, widthTexture, heightTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

			/*
			 * �ṩ���ƹ��ܡ������ö������鷽ʽ����ͼ��ʱ��ʹ�øú������ú������ݶ��������е��������ݺ�ָ����ģʽ�����л��ơ�
			       ���øú���֮ǰ��Ҫ������glEnableVertexAttribArray��glVertexAttribPointer�����ö������Ժ����ݡ�
			  mode�����Ʒ�ʽ��OpenGL2.0�Ժ��ṩ���²���:GL_POINTS��GL_LINES��GL_LINE_LOOP��GL_LINE_STRIP��GL_TRIANGLES��GL_TRIANGLE_STRIP��GL_TRIANGLE_FAN��
		      first�������黺���е���һλ��ʼ���ƣ�һ��Ϊ0��      count�������ж����������
			 */
			glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);//ֻ�ܴ�4��4������

			glDisableVertexAttribArray( ATTRIBUTE_VERTEX );
			glDisableVertexAttribArray( ATTRIBUTE_TEXCOORD );
			glBindTexture( GL_TEXTURE_2D, 0 );//������ȥ��

			//delete []rgbbuffer;
		}
	}

	unsigned int OpenGLRender::_CompileShader( unsigned int type, const char *shaderSource )
	{
		int status = 0;

		unsigned int shader = glCreateShader( type );//������ɫ��shader��Ŀǰһ����������ɫ��:������ɫ����Ƭ����ɫ��,������ɫ����ʶ��
		if( ( shader == 0 ) || ( shader == GL_INVALID_ENUM ) )
		{
			return 0;
		}

		/*����ɫ������Դ�����ַ�����󶨵���ɫ�����󣬵ڶ���������ʾָ���ַ�ָ��������Ԫ�صĸ����������ٸ���ɫ��Դ����.
		 * ����������Ϊ�ַ�ָ�������ַ�����ĸ������Ǹ�����ָ�����飬��shader�ַ�ָ�������Ӧ����ָ��ÿ��shaderԴ������ַ������ȡ�
		 * ����ΪNULL��ʾԴ�����ַ�����������ĸ���Ϊ0��
		 *Ϊ��ʹ����򵥣��ڱ����У��ַ�ָ������Ԫ��ֻ��һ������ֻ��һ��shaderSource����Դ���롣
		 */
		glShaderSource( shader, 1, &shaderSource, NULL );
		glCompileShader( shader );//������ɫ������

		//���ڻ����ɫ�����Ա����״̬
		glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
		if( status == GL_FALSE )
		{
			glDeleteShader( shader );//ɾ����ɫ��
			return 0;
		}

		return shader;
	}

	int OpenGLRender::_RegenTextures()
	{
		if( textureId )
		{
			glDeleteTextures( 1, &textureId );
		}
		glGenTextures( 1, &textureId );
		return 0;
	}

	/*
	 * ������ɫ��
	 */
	int OpenGLRender::_LoadShader()
	{
		int err = 0;

		do
		{
			program = glCreateProgram();//����һ����ɫ������,���ظ���ɫ������ı�ʶ��

			/*���ض�Ӧ����ɫ��
			 * ��Ҫ�����Ӧ����ɫ������
			 */
			vertexShader = _CompileShader( GL_VERTEX_SHADER, VERTEX_SHADER_STRING );
			if( !vertexShader )
			{
				err = -1;
				break;
			}

			fragmentShader = _CompileShader( GL_FRAGMENT_SHADER, YUV_FRAGMENT_SHADER_STRING );
			if( !fragmentShader )
			{
				err = -2;
				break;
			}

			//������õ�shader���ӵ�������
			glAttachShader( program, vertexShader );
			glAttachShader( program, fragmentShader );
			/*
			 *��Ӧ�ó����� ʹ��glBindAttribLocation�󶨶�����ɫ�������е�attribute����,��������Ⱦʱ��ʹ�ú���glVertexAttribPointer����
			 *��Ӧ�ó����� Ϊÿ��attribute������ֵ
			 */
			glBindAttribLocation( program, ATTRIBUTE_VERTEX, "position" );
			glBindAttribLocation( program, ATTRIBUTE_TEXCOORD, "texcoord" );

			//���ӳ���
			glLinkProgram( program );

			int status;
			glGetProgramiv( program, GL_LINK_STATUS, &status );//��ȡ���������״̬��Ϣ
			if( status == GL_FALSE )
			{
				err = -3;
				break;
			}

			glValidateProgram( program );//�ԣ���ɫ��������������ȷ����֤
			glGetProgramiv( program, GL_VALIDATE_STATUS, &status );//��ȡ�������ȷ����֤״̬��Ϣ
			if( status == GL_FALSE )
			{
				err = -4;
				break;
			}

			/*
			 * ��Ӧ�ó����л��Ƭ����ɫ���е�Uniform���ͱ�����λ��,�൱������Ӧ�ó�����ʹ�ñ�������Ƭ����ɫ���е�Uniform���ͱ�����
			 * ������ʵ����Ⱦʱ����Ӧ�ó����ж��丳ֵ
			 */
			texture = glGetUniformLocation( program, "s_texture" );
			texturesize = glGetUniformLocation( program, "TexSize" );
			enablegaussfilter = glGetUniformLocation( program, "enablegaussfilter" );
		}
		while( 0 );

		if( err < 0 )
		{
			if( vertexShader )
			{
				glDeleteShader( vertexShader );//ɾ��������ɫ��
				vertexShader = 0;
			}

			if( fragmentShader )
			{
				glDeleteShader( fragmentShader );//ɾ��Ƭ����ɫ��
				fragmentShader = 0;
			}

			if( program )
			{
				glDeleteProgram(program);//ɾ������
				program = 0;
			}
		}

		return err;
	}

	void OpenGLRender::Cleanup()
	{
		if( vertexShader )
		{
			glDeleteShader( vertexShader );//ɾ��������ɫ��
			vertexShader = 0;
		}

		if( fragmentShader )
		{
			glDeleteShader( fragmentShader );//ɾ��Ƭ����ɫ��
			fragmentShader = 0;
		}

		if( program )
		{
			glDeleteProgram(program);//ɾ������
			program = 0;
		}
	}
}



