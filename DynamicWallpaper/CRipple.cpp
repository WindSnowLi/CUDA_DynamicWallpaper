
#include "pch.h"
#include "CRipple.h"
#include <Windows.h>

/*
*
*
*@��2020/03/19���ԣ�ֱ�Ӹ���cudaMallocManaged�������ڴ�ռ䲻��ȡ��
*�ص�����һֱ���ظ���ִ��CUDA��Ⱦ���̣߳�ֱ�Ӹ���CUDA������ڴ����ɶ�д�쳣
*
*
*
*
/



/**
 * ���ܣ�ˮ����ʱ���ص���������ʱ���ص��������ܷŵ����Ա��
 * ������
 *		[in]		hWnd
 *		[in]		uMsg
 *		[in]		idEvent
 *		[in]		dwTime
 * ����ֵ��
 *		void
 */
bool allTerminateCallBack=true;
static void CALLBACK WaveTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	CRipple* pRipple = (CRipple*)idEvent;

	//ˮ����ɢ����Ⱦ
	pRipple->WaveSpread();
	pRipple->WaveRender();

	HDC hDc = GetDC(hWnd);

	//ˢ�µ���Ļ
	pRipple->UpdateFrame(hDc);
	ReleaseDC(hWnd, hDc);
}

//////////////////////////////////////////////////////////////////////
CRipple::CRipple()
{
	m_hWnd = NULL;
	m_hRenderDC = NULL;
	m_hRenderBmp = NULL;
	m_iBmpWidth = 0;
	m_iBmpHeight = 0;
	m_iBytesPerWidth = 0;
	
	memset(&m_stBitmapInfo, 0, sizeof(m_stBitmapInfo));
}

CRipple::~CRipple()
{
	FreeRipple();
}

/**
 * ���ܣ���ʼ��ˮ������
 * ������
 *		[in]		hWnd		���ھ��
 *		[in]		hBmp		ˮ������ͼƬ���
 *		[in]		uiSpeed		��ʱ�����ʱ�䣨ˢ���ٶȣ�
 * ����ֵ��
 *		�ɹ�true��ʧ��false
 */

UINT tempUiSpeed;

bool CRipple::InitRipple(HWND hWnd, HBITMAP hBmp, UINT uiSpeed)
{
	m_hWnd = hWnd;
	BITMAP stBitmap;

	if (GetObject(hBmp, sizeof(stBitmap), &stBitmap) == 0)
	{
		return false;
	}

	//��ȡλͼ���ߡ�һ�е��ֽ���
	m_iBmpWidth = stBitmap.bmWidth;
	m_iBmpHeight = stBitmap.bmHeight;
	m_iBytesPerWidth = (m_iBmpWidth * 3 + 3) & ~3;		//24λλͼ��һ������ռ3���ֽڣ�һ�е����ֽ���ҪΪ4�ı���

	//���䲨�ܻ�����
	m_pWaveBuf1 = new int[m_iBmpWidth * m_iBmpHeight];
	m_pWaveBuf2 = new int[m_iBmpWidth * m_iBmpHeight];

	//�ռ����ʧ��
	//if (m_pWaveBuf1 == NULL || m_pWaveBuf2 == NULL)
	//	return false;

	memset(m_pWaveBuf1, 0, sizeof(int) * m_iBmpWidth * m_iBmpHeight);
	memset(m_pWaveBuf2, 0, sizeof(int) * m_iBmpWidth * m_iBmpHeight);

	//����λͼ�������ݻ�����
	m_pBmpSource = new BYTE[m_iBytesPerWidth * m_iBmpHeight];
	m_pBmpRender = new BYTE[m_iBytesPerWidth * m_iBmpHeight];

	//�ռ����ʧ��
	//if (m_pBmpSource == NULL || m_pBmpRender == NULL)
	//	return false;

	HDC hDc = GetDC(m_hWnd);

	//��ȾDC
	//�ú�������һ����ָ���豸���ݵ��ڴ��豸�����Ļ�����DC����
	m_hRenderDC = CreateCompatibleDC(hDc);				
	
	
	//�ú������ڴ�����ָ�����豸������ص��豸���ݵ�λͼ��
	//��CreateCompatibleBitmap����������λͼ����ɫ��ʽ���ɲ���hdc��ʶ���豸����ɫ��ʽƥ�䣬
	//��λͼ����ѡ�������ڴ��豸�����У������ڴ��豸���������ɫ�͵�ɫ����λͼ��
	m_hRenderBmp = CreateCompatibleBitmap(hDc, m_iBmpWidth, m_iBmpHeight);

	//SelectObject�������������Ժ������ú���ѡ��һ����ָ�����豸�����Ļ����У����¶����滻��ǰ����ͬ���͵Ķ���
	SelectObject(m_hRenderDC, m_hRenderBmp);

	//��ʼ��BITMAPINFO�ṹ
	m_stBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_stBitmapInfo.bmiHeader.biWidth = m_iBmpWidth;
	m_stBitmapInfo.bmiHeader.biHeight = -m_iBmpHeight;
	m_stBitmapInfo.bmiHeader.biPlanes = 1;
	m_stBitmapInfo.bmiHeader.biBitCount = 24;					//24λλͼ
	m_stBitmapInfo.bmiHeader.biSizeImage = 0;
	m_stBitmapInfo.bmiHeader.biCompression = BI_RGB;

	//������ʱ�ڴ�DC����ͼƬ
	HDC hMemDC = CreateCompatibleDC(hDc);
	SelectObject(hMemDC, hBmp);
	ReleaseDC(m_hWnd, hDc);

	//��ȡλͼ����
	//int GetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage)��
	/*GetDIBits������ȡָ������λͼ��λ��Ȼ������һ��DIB���豸�޹�λͼ��Device-Independent Bitmap��ʹ�õ�ָ����ʽ���Ƶ�һ���������С�
	*cScanLines��ָ��������ɨ��������
	*lpvBits��ָ����������λͼ���ݵĻ�������ָ�롣����˲���ΪNULL����ô��������λͼ��ά�����ʽ���ݸ�lpbi����ָ���BITMAPINFO�ṹ��
	*lpbi��ָ��һ��BITMAPINFO�ṹ��ָ�룬�˽ṹȷ�����豸����λͼ�����ݸ�ʽ��
	*uUsage��ָ��BITMAPINFO�ṹ��bmiColors��Ա�ĸ�ʽ��������Ϊ����ȡֵ��
	*DIB_PAL_COLORS����ɫ����ָ��ǰ�߼���ɫ���16λ����ֵ���鹹�ɡ�
	*DIB_RGB_COLORS����ɫ���ɺ졢�̡�����RGB������ֱ��ֵ���ɡ�
	*����ֵ�����lpvBits�����ǿգ����Һ������óɹ�����ô����ֵΪ��λͼ���Ƶ�ɨ��������
	*/
	GetDIBits(hMemDC, hBmp, 0, m_iBmpHeight, m_pBmpSource, &m_stBitmapInfo, DIB_RGB_COLORS);
	GetDIBits(hMemDC, hBmp, 0, m_iBmpHeight, m_pBmpRender, &m_stBitmapInfo, DIB_RGB_COLORS);

	//��ȡ��λͼ���ݣ��ͷ��ڴ�DC
	DeleteDC(hMemDC);

	//���ö�ʱ��
	tempUiSpeed = uiSpeed;
	return true;
}

void CRipple::startTimer() {
	
	//@��2020/03/19��ȡ���ظ����帳ֵ����������GPU����
	cudaMallocManaged(&AlpWave1, sizeof(int) * m_iBmpWidth * m_iBmpHeight);
	cudaMallocManaged(&AlpWave2, sizeof(int) * m_iBmpWidth * m_iBmpHeight);

	cudaMallocManaged(&tempM_pBmpRender, sizeof(BYTE) * m_iBytesPerWidth * m_iBmpHeight);
	cudaMallocManaged(&tempM_pBmpSource, sizeof(BYTE) * m_iBytesPerWidth * m_iBmpHeight);

	cudaMemcpy(AlpWave1, 0, sizeof(int) * m_iBmpWidth * m_iBmpHeight, cudaMemcpyHostToDevice);
	cudaMemcpy(AlpWave2, 0, sizeof(int) * m_iBmpWidth * m_iBmpHeight, cudaMemcpyHostToDevice);

	cudaMemcpy(tempM_pBmpSource, m_pBmpSource, sizeof(BYTE) * m_iBytesPerWidth * m_iBmpHeight, cudaMemcpyHostToDevice);
	cudaMemcpy(tempM_pBmpRender, m_pBmpRender, sizeof(BYTE) * m_iBytesPerWidth * m_iBmpHeight, cudaMemcpyHostToDevice);
	SetTimer(m_hWnd, (UINT_PTR)this, tempUiSpeed, WaveTimerProc);
}

void CRipple::cancelTimer() {
	KillTimer(m_hWnd, (UINT_PTR)this);
	//@��2020/03/19��ȡ���ظ����帳ֵ����������GPU����
	cudaFree(AlpWave1);
	cudaFree(AlpWave2);

	cudaFree(tempM_pBmpRender);
	cudaFree(tempM_pBmpSource);
}
/**
 * ���ܣ��ͷ�ˮ��������Դ
 * ������
 *		void
 * ����ֵ��
 *		void
 */
void CRipple::FreeRipple()
{
	//�ͷ���Դ
	if (m_hRenderDC != NULL)
	{
		DeleteDC(m_hRenderDC);
	}
	if (m_hRenderBmp != NULL)
	{
		DeleteObject(m_hRenderBmp);
	}
	
	if (m_pWaveBuf1 != NULL)
	{
		delete[]m_pWaveBuf1;
	}
	if (m_pWaveBuf2 != NULL)
	{
		delete[]m_pWaveBuf2;
	}
	if (m_pBmpSource != NULL)
	{
		delete[]m_pBmpSource;
	}
	if (m_pBmpRender != NULL)
	{
		delete[]m_pBmpRender;
	}
	
	//ɱ��ʱ��
	KillTimer(m_hWnd, (UINT_PTR)this);
}

/**
 * ���ܣ�ˮ����ɢ
 * ������
 *		void
 * ����ֵ��
 *		void
 */
void CRipple::WaveSpread()
{
	//@��2020/03/19��ȡ���ظ����帳ֵ����������GPU����
	ToCUDAWaveSpreadThreadStart(AlpWave1, AlpWave2, m_iBmpWidth, m_iBmpHeight);

	int* AlpWave = AlpWave1;
	AlpWave1 = AlpWave2;
	AlpWave2 = AlpWave;
}

/**
 * ���ܣ�����ˮ��������Ⱦˮ��λͼ����
 * ������
 *		void
 * ����ֵ��
 *		void
 */
void CRipple::WaveRender()
{
	//@��2020/03/19��ȡ���ظ����帳ֵ����������GPU����
	cudaMemcpy(tempM_pBmpSource, m_pBmpSource, sizeof(BYTE) * m_iBytesPerWidth * m_iBmpHeight, cudaMemcpyHostToDevice);
	ToCUDACUDAWaveRenderThreadStart(AlpWave1, tempM_pBmpRender, tempM_pBmpSource,m_iBytesPerWidth,m_iBmpWidth,m_iBmpHeight);
	///Ͷ�벨Դ��ͼƬ��Ⱦ�Լ����㲨�����̲߳��У����������޸�tempM_pBmpSource��ֵ������ԭ��δ֪������tempM_pBmpSource�ظ���ֵ�ݱ�
	
	//������Ⱦ���λͼ
	SetDIBits(m_hRenderDC, m_hRenderBmp, 0, m_iBmpHeight, tempM_pBmpRender, &m_stBitmapInfo, DIB_RGB_COLORS);
}

/**
 * ���ܣ�ˢ��ˮ����hDc��
 * ������
 *		[in]		hDc			ˢ��Ŀ��DC��һ��Ϊ��ĻDC��Ҫ��ʾ�����
 * ����ֵ��
 *		void
 */


void CRipple::UpdateFrame(HDC hDc)
{
	//BitBlt(hDc, 0, 0, m_iBmpWidth, m_iBmpHeight, m_hRenderDC, 0, 0, SRCCOPY);
	HDC goalDC = ::GetDC(workerw);
	BitBlt(goalDC, 0, 0, m_iBmpWidth, m_iBmpHeight, m_hRenderDC, 0, 0, SRCCOPY);
	DeleteDC(goalDC);

}

/**
 * ���ܣ���ʯ�ӣ��趨��Դ��
 * ������
 *		[in]		x				ʯ��λ��x
 *		[in]		y				ʯ��λ��y
 *		[in]		stoneSize		ʯ�Ӵ�С���뾶��
 *		[in]		stoneWeight		ʯ������
 * ����ֵ��
 *		void
 */

void CRipple::DropStone(int x, int y, int stoneSize, int stoneWeight)
{	
	ToModifyCUDALpWaveThreadStart(AlpWave2, m_iBmpWidth, m_iBmpHeight, x, y, stoneSize, stoneWeight);
}