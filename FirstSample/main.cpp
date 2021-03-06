#include "GobalVariables.h"
#include "HalconCpp.h"
#include "HDevThread.h"
#include <conio.h>
#include <iomanip>
#include <thread>
#include <ConsumerImplHelper/ToFCamera.h>
#include "Execute.h"
#include "Task.h"
// 宏定义
////
//摄像头宏定义
#if defined (_MSC_VER) && defined (_WIN32)
// You have to delay load the GenApi libraries used for configuring the camera device.
// Refer to the project settings to see how to instruct the linker to delay load DLLs. 
// ("Properties->Linker->Input->Delay Loaded Dlls" resp. /DELAYLOAD linker option).
#  pragma message( "Remember to delayload these libraries (/DELAYLOAD linker option):")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GCBase") "\"")
#  pragma message( "    /DELAYLOAD:\"" DLL_NAME("GenApi") "\"")
#endif

//命名空间
using namespace HalconCpp;
using namespace GenTLConsumerImplHelper;
using namespace std;

//全局变量
rc17::Execute myExe;
HObject depthImage, confidenceImage;
const void *depthData;//深度图像数据
const void *intensityData;
const void *confidenceData;
bool startFromExtern = 0;
char szWrite[] = "Runing...";
DWORD dwWrite;
HANDLE hPipe;

double getFrameRate()
{
	static clock_t now = clock();
	static bool firstCalled = true;
	if (!firstCalled)
	{
		//while (clock() - now < 55);
		clock_t newNow = clock();
		int distanceMs = newNow - now;
		double frameRate = 1000.0 / distanceMs;
		now = newNow;
		return frameRate;
	}
	else
	{
		firstCalled = false;
		return -1;
	}
}

class CameraAction
{
public:
    int run();
    bool onImageGrabbed( GrabResult grabResult, BufferParts );
	void imageConfig();

private:
    CToFCamera  m_Camera;
    int         m_nBuffersGrabbed;
};

void CameraAction::imageConfig(void)
{
	// Enable 3D (point cloud) data, intensity data, and confidence data 
	GenApi::CEnumerationPtr ptrComponentSelector = m_Camera.GetParameter("ComponentSelector");
	GenApi::CBooleanPtr ptrComponentEnable = m_Camera.GetParameter("ComponentEnable");
	GenApi::CEnumerationPtr ptrPixelFormat = m_Camera.GetParameter("PixelFormat");

	// Enable range data
	ptrComponentSelector->FromString("Range");
	ptrComponentEnable->SetValue(true);
	// Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this CameraAction, we want to acquire 3D coordinates.
	// Note: To change the format of an image component, the Component Selector must first be set to the component
	// you want to configure (see above).
	// To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
	ptrPixelFormat->FromString("Coord3D_ABC32f");

	ptrComponentSelector->FromString("Intensity");
	ptrComponentEnable->SetValue(true);

	ptrComponentSelector->FromString("Confidence");
	ptrComponentEnable->SetValue(true);


	//修改各项参数
	/*image quality*/

	GenApi::CIntegerPtr ptrConfidenceThreshold = m_Camera.GetParameter("ConfidenceThreshold");
	ptrConfidenceThreshold->SetValue(1008);
	GenApi::CIntegerPtr ptrFilterStrength = m_Camera.GetParameter("FilterStrength");
	ptrFilterStrength->SetValue(50);
	GenApi::CIntegerPtr ptrOutlierTolerance = m_Camera.GetParameter("OutlierTolerance");
	ptrOutlierTolerance->SetValue(17088);

	/*图像格式控制*/

	//GenApi::CIntegerPtr ptrWidth = m_Camera.GetParameter("Width");
	//ptrWidth->SetValue(640);
	//GenApi::CIntegerPtr ptrHeight = m_Camera.GetParameter("Height");
	//ptrHeight->SetValue(480);
	//GenApi::CIntegerPtr ptrOffsetX = m_Camera.GetParameter("OffsetX");
	//ptrOffsetX->SetValue(0);
	//GenApi::CIntegerPtr ptrOffsetY = m_Camera.GetParameter("OffsetY");
	//ptrOffsetY->SetValue(0);

	/*深度阈值*/

	GenApi::CIntegerPtr ptrDepthMin = m_Camera.GetParameter("DepthMin");
	ptrDepthMin->SetValue(0);
	GenApi::CIntegerPtr ptrDepthMax = m_Camera.GetParameter("DepthMax");
	ptrDepthMax->SetValue(13000);

	/*曝光时间*/

	GenApi::CEnumerationPtr ptrExposureAuto = m_Camera.GetParameter("ExposureAuto");
	ptrExposureAuto->FromString("Continuous");

	//GenApi::CIntegerPtr ptrExposureTimeSelector = m_Camera.GetParameter("ExposureTimeSelector"); ptrExposureTimeSelector->SetValue(0);

	//GenApi::CFloatPtr ptrExposureTime = m_Camera.GetParameter("ExposureTime");
	//ptrExposureTime->SetValue(10000);

	/*温度（可选传感器还是LED）*/

	//GenApi::CEnumerationPtr ptrDeviceTemperatureSelector = m_Camera.GetParameter("DeviceTemperatureSelector"); ptrDeviceTemperatureSelector->FromString("SensorBoard");//"LEDBoard"
	//std::cout << "Value of DeviceTemperatureSelector is " << ptrDeviceTemperatureSelector->ToString() << std::endl;

	/*帧数*/
	GenApi::CFloatPtr ptrAcquisitionFrameRate = m_Camera.GetParameter("AcquisitionFrameRate");
	ptrAcquisitionFrameRate->SetValue(20);//最大为20帧
	//std::cout << "Value of AcquisitionFrameRate is " << ptrAcquisitionFrameRate->GetValue() << std::endl;
}

int CameraAction::run()
{
    m_nBuffersGrabbed = 0;

    try
    {
        m_Camera.OpenFirstCamera();
        cout << "Connected to camera " << m_Camera.GetCameraInfo().strDisplayName << endl;

		imageConfig();
        // Acquire images until the call-back onImageGrabbed indicates to stop acquisition. 
        // 5 buffers are used (round-robin).
        m_Camera.GrabContinuous( 5, 1000, this, &CameraAction::onImageGrabbed );

        // Clean-up
        m_Camera.Close();

    }
    catch ( const GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << e.GetDescription() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool CameraAction::onImageGrabbed( GrabResult grabResult, BufferParts parts )
{
	if(startFromExtern)
		WriteFile(hPipe, szWrite, 10, &dwWrite, NULL);
	/********************帧率显示**********************/
	//getFrameRate();
	//cout << "frameRate:  " << setw(4) << getFrameRate() << endl;
	/*************************************************/

	if (grabResult.status == GrabResult::Timeout)
	{
		cerr << "Timeout occurred. Acquisition stopped." << endl;
		return false; // Indicate to stop acquisition
	}
	m_nBuffersGrabbed++;
	if (grabResult.status != GrabResult::Ok)
	{
		cerr << "Image " << m_nBuffersGrabbed << "was not grabbed." << endl;
	}
	else
	{
		// Retrieve the values for the center pixel
		const int width = (int)parts[0].width;
		const int height = (int)parts[0].height;

		//生成深度图像
		depthData = parts[0].pData;
		intensityData = parts[1].pData;
		confidenceData = parts[2].pData;
		//float *depthImageByte = new float[width*height];
		uint16_t *depthImageByte = new uint16_t[width*height];
		for (int i = 0; i < width*height; i++)
		{
			if (((uint16_t*)confidenceData)[i] < 4000 && (((myCoor3D*) depthData + i)->z) < 4000)
				depthImageByte[i] = 0;
			else           
				depthImageByte[i] = (((myCoor3D*) depthData + i)->z);
		}
		GenImage1(&depthImage, "uint2", width, height, (Hlong)(depthImageByte));
		GenImage1(&confidenceImage, "uint2", width, height, (Hlong)(confidenceData));
		//释放图片内存
		delete[] depthImageByte;
		/********************防止帧重复************************/
		static HObject lastImage = depthImage;
		HObject tmp, emptyRegion;
		GenEmptyRegion(&emptyRegion);
		DynThreshold(depthImage, lastImage, &tmp, 5, "not_equal");
		lastImage = depthImage;
		if (tmp == emptyRegion)
			return 1;
		/******************************************************/
		myExe.run(depthImage);
		//if (HDevWindowStack::IsOpen())
		//	DispObj(confidenceImage, HDevWindowStack::GetActive());
		//WriteImage(confidenceImage, "tiff", 0, "C:/Users/robocon2017/Desktop/2/confidence" );
		//WriteImage(depthImage, "tiff", 0, "C:/Users/robocon2017/Desktop/2/depth");
		/*****************************************************************/

			
		HTuple  hv_Row, hv_Column, hv_Button;
		HTuple  hv_Grayval, hv_Exception;

		/*按鼠标中键退出程序*/
		rc17::ThreadFlag::run = true;
		try
		{
			GetMposition(rc17::HalconVar::hv_WindowHandle, &hv_Row, &hv_Column, &hv_Button);
			GetGrayval(depthImage, hv_Row, hv_Column, &hv_Grayval);
			//myCoor3D *p3DCoordinate = (myCoor3D*) depthData + (int)hv_Row.D() * width + (int)hv_Column.D();

			if (0 != (hv_Button == 2))
			{
				rc17::ThreadFlag::run = false;
			}
			if (0 != (hv_Button == 1))
			{
				cout << "Row: " << hv_Row.D() << "    Column: " << hv_Column.D() << endl;
			}
		}
		// catch (Exception) 
		catch (HalconCpp::HException &HDevExpDefaultException)
		{
			HDevExpDefaultException.ToHTuple(&hv_Exception);
		}

	}
		
	return rc17::ThreadFlag::run;
}



int main(int argc, char* argv[])
{
	if (argv[argc - 1][0] == '-' || argv[argc - 1][1] == 'e')
		startFromExtern = 1;
	if (startFromExtern)
	{
		LPCWSTR pStrPipeName = TEXT("\\\\.\\pipe\\pipe");
		hPipe = CreateFile(pStrPipeName, GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	
    int exitCode = EXIT_SUCCESS;
	myExe.init();
	//std::thread t_Correct(rc17::Correct);
    try
    {
        CToFCamera::InitProducer();

        CameraAction processing;

        exitCode = processing.run();
    }
    catch ( GenICam::GenericException& e )
    {
        cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
        exitCode = EXIT_FAILURE;
    }

    if ( CToFCamera::IsProducerInitialized() )
        CToFCamera::TerminateProducer();  // Won't throw any exceptions
	{
		//t_Correct.join();//等待该线程结束
		return exitCode;
	}
}
