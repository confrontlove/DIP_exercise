#include<opencv2\imgproc\imgproc.hpp>
#include<iostream>
#include<algorithm>
#include<vector>

//��Сֵ�˲����ø�ʴ������
void min_filter(cv::Mat &src_img, cv::Mat &res_img, int kernel_size);

//BGRͨ������Сֵ
cv::Mat min_BGR(cv::Mat &src_img);

//���Լ�д�ĵ����˲�
cv::Mat guide_filter(cv::Mat &img, cv::Mat p, int r, double eps);


//������������������˲��ĺ�����opencv contrib��ģ��п��ٰ汾����û�����ؿ����������ҵģ�
//https://blog.csdn.net/wds555/article/details/23176313

cv::Mat GuidedFilter(cv::Mat I, cv::Mat p, int r, double eps);
cv::Mat fastGuidedFilter(cv::Mat I_org, cv::Mat p_org, int r, double eps, int s);

//�����������ͼ��V1�͹���ֵ
void getV1(cv::Mat &m, int r, double eps, double w, double maxV1, double &A, cv::Mat &V1_);

//���յ�ȥ������������ԭʼͼ�񣬷���ȥ��֮���ͼ��
cv::Mat deHaze(cv::Mat &img, double r = 81, double eps = 0.001, double w = 0.95, double maxV1 = 0.80, bool Gamma = false);