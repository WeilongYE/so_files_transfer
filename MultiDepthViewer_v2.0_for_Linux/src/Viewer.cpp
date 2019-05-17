
// Undeprecate CRT functions
#ifndef _CRT_SECURE_NO_DEPRECATE 
	#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "Viewer.h"
#include <iostream>
#include <chrono>
#include <unistd.h>

#include <GL/glut.h>
#include "OniSampleUtilities.h"

#define GL_WIN_SIZE_X	640
#define GL_WIN_SIZE_Y	480
#define TEXTURE_SIZE	480

#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_DEPTH1

#define MIN_NUM_CHUNKS(data_size, chunk_size)	((((data_size)-1) / (chunk_size) + 1))
#define MIN_CHUNKS_SIZE(data_size, chunk_size)	(MIN_NUM_CHUNKS(data_size, chunk_size) * (chunk_size))

SampleViewer* SampleViewer::ms_self = NULL;
openni::VideoFrameRef g_depthFrame;
char		*tempSiralNum;

void SampleViewer::glutIdle()
{
	glutPostRedisplay();
}

void SampleViewer::glutDisplay()
{
       
	SampleViewer::ms_self->display();
}

void SampleViewer::glutKeyboard(unsigned char key, int x, int y)
{
	SampleViewer::ms_self->onKey(key, x, y);
}

SampleViewer::SampleViewer(const char* strSampleName, openni::VideoStream depthStreams[], int devNum, COBDevice* cob_devices) :
m_pTexMap(NULL), m_depthIndex(0), m_devNum(devNum)

{
    m_depthStreams = depthStreams;
	ms_self = this;
  cob_devices_ = cob_devices;
	strncpy(m_strSampleName, strSampleName, ONI_MAX_STR);
}

SampleViewer::~SampleViewer()
{
	delete[] m_pTexMap;

	ms_self = NULL;
}

openni::Status SampleViewer::init(int argc, char **argv, const char** deviceUri, char(*serialNum)[12])
{
    m_serialNum = serialNum;
	m_deviceUri = deviceUri;

	m_nResX = m_depthStreams[0].getVideoMode().getResolutionX();
	m_nResY = m_depthStreams[0].getVideoMode().getResolutionY();

	// Texture map init
	m_nTexMapX = MIN_CHUNKS_SIZE(m_nResX, TEXTURE_SIZE);
	m_nTexMapY = MIN_CHUNKS_SIZE(m_nResY, TEXTURE_SIZE);
	m_pTexMap = new openni::RGB888Pixel[m_nTexMapX * m_nTexMapY];

    for (int i = 0; i < m_devNum; ++i)
    {
        if (i == m_depthIndex)
            continue;
        switchLaserByUri(m_deviceUri[i], true);
    }

	return initOpenGL(argc, argv); 
}

void SampleViewer::switchLaserByUri(const int id, bool turnOn)
{
  if (turnOn)
	{
		*(uint16_t*)buf1 = 1;
	}
	else
	{
		*(uint16_t*)buf1 = 0;
	}
	
	cob_devices_[id].SendCmd(85, buf1, 2, buf2, 2);
}

void SampleViewer::switchLaserByUri(const char* uri, bool turnOn)
{
	COBDevice  device;
	device.InitDevice();
	device.OpenDevice(uri);

	if (turnOn)
	{
		*(uint16_t*)buf1 = 1;
	}
	else
	{
		*(uint16_t*)buf1 = 0;
	}
	
	device.SendCmd(85, buf1, 2, buf2, 2);
	device.CloseDevice();
}

//according to key index to operate Laser 
void SampleViewer::operateLaser(int index)
{
	if (index >= 1 && index <= m_devNum)
	{
		index -= 1; // Convert to array index
		if (m_depthIndex != index)
		{
			switchLaserByUri(m_deviceUri[m_depthIndex], false); // turn off last laser
			m_depthIndex = index;
			switchLaserByUri(m_deviceUri[m_depthIndex], true); // turn on next laser
			std::cout << "Switched to device " << index + 1 << " SN: " << m_serialNum[index] << std::endl;
		}
		else
		{
			std::cout << "Switched to device " << index + 1 << " SN: " << m_serialNum[index] << std::endl;
		}
	}
}

openni::Status SampleViewer::run()	//Does not return
{
	glutMainLoop();
   
	return openni::STATUS_OK;
}

void SampleViewer::displayFrame(const openni::VideoFrameRef& frame)
{
	if (!frame.isValid())
		return;

	const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)frame.getData();
	openni::RGB888Pixel* pTexRow = m_pTexMap + frame.getCropOriginY() * m_nTexMapX;
	int rowSize = frame.getStrideInBytes() / sizeof(openni::DepthPixel);

	for (int y = 0; y < frame.getHeight(); ++y)
	{
		const openni::DepthPixel* pDepth = pDepthRow;
		openni::RGB888Pixel* pTex = pTexRow + frame.getCropOriginX();

		for (int x = 0; x < frame.getWidth(); ++x, ++pDepth, ++pTex)
		{
			if (*pDepth != 0)
			{
				int nHistValue = m_pDepthHist[*pDepth];
				pTex->r = nHistValue;
				pTex->g = nHistValue;
				pTex->b = nHistValue;
			}
		}

		pDepthRow += rowSize;
		pTexRow += m_nTexMapX;
	}
}

void SampleViewer::display()
{
  display(0);
  //display(1); 
}

void SampleViewer::display(const int id)
{
  m_depthIndex = id;
	switchLaserByUri(1-m_depthIndex, true);
	switchLaserByUri(m_depthIndex,false );
  //usleep(6*1000);

	// Read all frames
    //for (int i = 0; i < m_devNum; ++i)
    //{
    //    m_depthStreams[i].readFrame(&(m_depthFrame[i]));
    //}

  //m_depthIndex = 1;
  m_depthStreams[m_depthIndex].readFrame(&(m_depthFrame[m_depthIndex]));

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, GL_WIN_SIZE_X, GL_WIN_SIZE_Y, 0, -1.0, 1.0);

	memset(m_pTexMap, 0, m_nTexMapX * m_nTexMapY * sizeof(openni::RGB888Pixel));

	// Display selected frame
	int depthIndex = m_depthIndex;
	calculateHistogram(m_pDepthHist, MAX_DEPTH, m_depthFrame[depthIndex]);

	displayFrame(m_depthFrame[depthIndex]);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nTexMapX, m_nTexMapY, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pTexMap);

	// Display the OpenGL texture map
	glColor4f(1,1,1,1);

	glBegin(GL_QUADS);

	// upper left
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	// upper right
	glTexCoord2f((float)m_nResX / (float)m_nTexMapX, 0);
	glVertex2f(GL_WIN_SIZE_X, 0);
	// bottom right
	glTexCoord2f((float)m_nResX / (float)m_nTexMapX, (float)m_nResY / (float)m_nTexMapY);
	glVertex2f(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	// bottom left
	glTexCoord2f(0, (float)m_nResY / (float)m_nTexMapY);
	glVertex2f(0, GL_WIN_SIZE_Y);

	glEnd();

	// Swap the OpenGL display buffers
	glutSwapBuffers();

  m_depthIndex = id;
	switchLaserByUri(m_depthIndex, false);
	switchLaserByUri(1-m_depthIndex, true);
  usleep(70*1000);
}

void SampleViewer::onKey(unsigned char key, int /*x*/, int /*y*/)
{
	switch (key)
	{
	case 27:
        for (int i = 0; i < m_devNum; i++)
        {
            m_depthStreams[i].stop();
            m_depthStreams[i].destroy();
        }
        
		openni::OpenNI::shutdown();
		exit (1);
        break;
        case 'o':
                while (true){
		for (int i = 1; i < m_devNum+1; i++)
		{
			operateLaser(i);
			//std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        usleep(166667);
                        
			SampleViewer::display(); // call the display()
			g_depthFrame = m_depthFrame[i-1];
			tempSiralNum = *(m_serialNum+i-1);
			//captureSingleFrame(tempSiralNum);
		}
                }
		break;

	default:

               int index = key - 48; // char '0' is 48
	       std::cout << "key " << index << std::endl;
	       operateLaser(index);
        break;
             
              //for (int index = 1; index <= m_devNum; index++)
              //{  
              /* int index = key - 48;
               std::cout << "Key " << index << std::endl;
               std::cout << "DepthIndex: "<< m_depthIndex<< std::endl;
               while (index == (m_depthIndex+1)){
  
                  if (index == 2){
                     index = 1;

                 }
                 else{
                     index = 2;

                 }

		//int index = key - 48; // char '0' is 48
		std::cout << "Key " << index << std::endl;
                int counter = 1;

		if (index >= 1 && index <= m_devNum)
		{
			index -= 1; // Convert to array index

			if (m_depthIndex != index)
			{
                              while (true){
				switchLaserByUri(m_deviceUri[m_depthIndex], false); // turn off last laser
				m_depthIndex = index;
				switchLaserByUri(m_deviceUri[m_depthIndex], true); // turn on next laser
				std::cout << "Switched to device " << index+1 << " SN: " << m_serialNum[index] << std::endl;
                                //SampleViewer::glutDisplay();
                                
                                sleep(2);
                               
                                
                                if (index == 1){
                                  SampleViewer::display(index);
                                    
                                  
                                }
                                
                                if (index == 1){
                                    index = 0;

                                }
                                else{
                                    index = 1;

                               }
                                 

                               if (counter == 5){
                                 // break;
                                  counter=1;
                                }else{
                                 counter++;
                                } 
                              }

			}
		}
             
               

             }
           
        break;*/
	}
}

openni::Status SampleViewer::initOpenGL(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow (m_strSampleName);
	// 	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	initOpenGLHooks();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	return openni::STATUS_OK;

}

void SampleViewer::initOpenGLHooks()
{
	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);
}
