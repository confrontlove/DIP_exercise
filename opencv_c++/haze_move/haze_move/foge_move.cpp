#include"fog_move.hpp"
#include<opencv2\highgui\highgui.hpp>   //������


//��Сֵ�˲���ʵ���Ͼ���һ����ʴ�Ĺ��̣�ֱ�ӽ���opencv�ĸ�ʴ��������
void min_filter(cv::Mat &src_img, cv::Mat &res_img, int kernel_size)
{
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2*kernel_size-1, 2*kernel_size-1));
	cv::erode(src_img, res_img, element);
}

//��ȡͨ������Сֵ������ֻ���Լ��������ˣ�������ָ�����ģ�ò���������
cv::Mat min_BGR(cv::Mat &src_img)
{
	int rows = src_img.rows;
	int cols = src_img.cols;
	
	cv::Mat res = cv::Mat::zeros(cv::Size(cols, rows), CV_8UC1);
	//creat one channel image to save min_rgb
	int channel_num = src_img.channels();
	std::cout << channel_num << std::endl;
	if (channel_num <= 1)
	{
		std::cout << "channel is just 1, return zeros matrix!" << std::endl;
		return res;
	}
	
	
	for (int i = 0; i < rows; i++)
	{
		uchar *img_adress = src_img.ptr<uchar>(i);   //��ǰ�еĵ�ַ
		uchar *res_adress = res.ptr<uchar>(i);       //��ǰ�еĵ�ַ
		for (int j_i = 0,j_r=0; j_r < cols,j_i<cols*channel_num; j_i+=3,j_r++)
		{
			res_adress[j_r] =std::min(std::min(img_adress[j_i],img_adress[j_i+1]),img_adress[j_i+2]);
		}
		
	}
	std::cout << "done!"<<std::endl;
	return res;

}


//�����˲�����
cv::Mat guide_filter(cv::Mat &img, cv::Mat p, int r, double eps)
{
	cv::Mat img_32f, p_32f, res;
	img.convertTo(img_32f, CV_32F);
	p.convertTo(p_32f, CV_32F);
	//��ת����32F���������˷�


	cv::Mat i_p, i2;
	cv::multiply(img_32f, p_32f, i_p);
	cv::multiply(p_32f, p_32f, i2);

	int height = img.rows;
	int width = img.cols;
	cv::Size kernel_sz(2*r-1, 2*r-1);

	//�����ĸ���ֵ�˲���
	cv::Mat m_Img, m_p, m_Ip, m_i2;    
	cv::boxFilter(img_32f, m_Img, CV_32F, kernel_sz);
	cv::boxFilter(p_32f, m_p, CV_32F, kernel_sz);
	cv::boxFilter(img_32f.mul(p_32f), m_Ip, CV_32F, kernel_sz);
	cv::boxFilter(img_32f.mul(img_32f), m_i2, CV_32F, kernel_sz);
	
	//����ip��Э�����img�ķ���
	cv::Mat cov_ip = m_Ip - m_Img.mul(m_p);
	cv::Mat var_i = m_i2 - m_Img.mul(m_Img);

	//��a��b
	cv::Mat a, b;
	cv::divide(cov_ip, var_i+eps, a);
	//a = cov_ip / (var_i + eps);
	b = m_p - a.mul(m_Img);

	//�԰�������i������a,b��ƽ��(����a��b����ֵ�˲�)

	cv::Mat mean_a, mean_b;
	cv::boxFilter(a, mean_a, CV_32F, kernel_sz);
	cv::boxFilter(b, mean_b, CV_32F, kernel_sz);

	//�������
	res = mean_a.mul(img_32f) + mean_b;
	
	return res;	
}

//getV1��A������m��Uint8���ͣ�ת������������
void getV1(cv::Mat &m,int r, double eps,double w,double maxV1,double &A,cv::Mat &V1_)
{
	cv::Mat m_32f;
	m.convertTo(m_32f, CV_32F);
	m_32f = m_32f / 255.0;

	cv::Mat V1 = min_BGR(m);    
	cv::Mat V1_32f;
	V1.convertTo(V1_32f, CV_32F);
	V1_32f = V1_32f / 255.0;

	cv::Mat V1_min_filter;
	min_filter(V1, V1_min_filter, 7);
	cv::Mat V1_min_32f;
	V1_min_filter.convertTo(V1_min_32f, CV_32F);        
	//��Ϊ��д��min_bgr���������uint8�ģ����Ծ���������Сֵ֮����ȥ����255
	V1_min_32f = V1_min_32f / 255.0;

	cv::Mat V1_g = guide_filter(V1_32f, V1_min_32f, r, eps);
	//�����˲��԰�ͨ��Ϊԭͼ����Сֵ�˲�֮��ͨ����ͼ��Ϊ����
	cv::imshow("�����˲����", V1_g);
	std::cout << V1_g(cv::Rect(0, 0, 3, 3)) << std::endl;
	double max, min;
	cv::minMaxLoc(V1_g, &min, &max, NULL, NULL);
	/*std::cout << max << "  " << min << std::endl;*/


	int bins = 2000;
	const int channels[1] = { 0 };
	float midRanges[] = { 0,max };
	int hist_sz[] = { bins };
	cv::Mat dstHist;
	const float *ranges[] = { midRanges };
	
	
	cv::calcHist(&V1_g,1,channels,cv::Mat(),dstHist,1,hist_sz,ranges,true,false);        //ͳ��ֱ��ͼ
	

	//std::cout << dstHist;
	cv::Mat sum_hist;
	cv::integral(dstHist, sum_hist,CV_32F);      //���������ͼ
	cv::Mat sum_hist2 = sum_hist(cv::Rect(1, 1, 1, 1000))/ V1_g.total();      //�ѵڶ���ȡ����
	
	//std::cout << sum_hist2;

	//����vector������
	std::vector<float> vd;
	vd.assign((float*)sum_hist2.datastart, (float*)sum_hist2.dataend);
	auto it_=std::upper_bound(vd.begin(), vd.end(),0.999);

	double value_0999 = (max - min) / bins*(it_ - vd.begin());   
	//value_0999����˵С��value_0999������ռ��������99.9%
	std::cout << value_0999;

	//������ֵ������ȡ��Ĥȥ��V1����������ֵ
	cv::Mat mask_up_value_0999;
	cv::threshold(V1_g, mask_up_value_0999, value_0999,1,CV_THRESH_BINARY);


	//��������ͨ���ľ�ֵ
	std::vector<cv::Mat> bgr;
	cv::split(m_32f, bgr);
	cv::Mat bgr_mean = (bgr[0] + bgr[1] + bgr[2]) / 3;

	//��ֵ��Ĥ
	cv::Mat A_ = bgr_mean.mul(mask_up_value_0999);
	
	cv::minMaxLoc(A_, NULL, &A, NULL, NULL);
	//�������ͨ���ˣ���python��������ֵ����ȫһ����
	
	cv::threshold(V1_g, V1_, maxV1, NULL,CV_THRESH_TRUNC);
}


cv::Mat deHaze(cv::Mat &img,double r , double eps , double w , double maxV1 , bool Gamma )
{
	cv::Mat V1,img_32f;
	double A;
	getV1(img, r, eps, w, maxV1, A, V1);
	cv::imshow("V1", V1);
	std::cout << A << std::endl;

	img.convertTo(img_32f, CV_32F);
	img_32f = img_32f / 255.0;


	//����ͨ������ֱ����
	std::vector<cv::Mat>  img_bgr;
	cv::split(img_32f, img_bgr);     
	cv::Mat Yb = (img_bgr[0] - V1) / (1 - V1 / A);
	cv::Mat Yg = (img_bgr[1] - V1) / (1 - V1 / A);
	cv::Mat Yr = (img_bgr[2] - V1) / (1 - V1 / A);
	
	//����ͨ���ϲ�
	img_bgr.clear();
	img_bgr.push_back(Yb);
	img_bgr.push_back(Yg);
	img_bgr.push_back(Yr);
	cv::Mat Y;
	cv::merge(img_bgr, Y);
	cv::threshold(Y, Y, 1, 1, CV_THRESH_BINARY);   //����1 ��ȫ����1
	cv::threshold(Y, Y, 0, 0, CV_THRESH_TOZERO);
	Y = Y*255.0;
	cv::Mat Y_;
	Y.convertTo(Y_, CV_8U);
	return Y_;


}

cv::Mat fastGuidedFilter(cv::Mat I_org, cv::Mat p_org, int r, double eps, int s)
{
	/*
	% GUIDEDFILTER   O(N) time implementation of guided filter.
	%
	%   - guidance image: I (should be a gray-scale/single channel image)
	%   - filtering input image: p (should be a gray-scale/single channel image)
	%   - local window radius: r
	%   - regularization parameter: eps
	*/

	cv::Mat I, _I;
	I_org.convertTo(_I, CV_32F);

	cv::resize(_I, I,cv::Size(), 1.0 / s, 1.0 / s, 1);



	cv::Mat p, _p;
	p_org.convertTo(_p, CV_32F);
	//p = _p;  
	cv::resize(_p, p, cv::Size(), 1.0 / s, 1.0 / s, 1);

	//[hei, wid] = size(I);      
	int hei = I.rows;
	int wid = I.cols;

	r = (22 * r + 1) / s + 1;//��Ϊopencv�Դ���boxFilter�����е�Size,����9x9,����˵�뾶Ϊ4     

							 //mean_I = boxfilter(I, r) ./ N;      
	cv::Mat mean_I;
	cv::boxFilter(I, mean_I, CV_32F, cv::Size(r, r));

	//mean_p = boxfilter(p, r) ./ N;      
	cv::Mat mean_p;
	cv::boxFilter(p, mean_p, CV_32F, cv::Size(r, r));

	//mean_Ip = boxfilter(I.*p, r) ./ N;      
	cv::Mat mean_Ip;
	cv::boxFilter(I.mul(p), mean_Ip, CV_32F, cv::Size(r, r));

	//cov_Ip = mean_Ip - mean_I .* mean_p; % this is the covariance of (I, p) in each local patch.      
	cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p);

	//mean_II = boxfilter(I.*I, r) ./ N;      
	cv::Mat mean_II;
	cv::boxFilter(I.mul(I), mean_II, CV_32F, cv::Size(r, r));

	//var_I = mean_II - mean_I .* mean_I;      
	cv::Mat var_I = mean_II - mean_I.mul(mean_I);

	//a = cov_Ip ./ (var_I + eps); % Eqn. (5) in the paper;         
	cv::Mat a = cov_Ip / (var_I + eps);

	//b = mean_p - a .* mean_I; % Eqn. (6) in the paper;      
	cv::Mat b = mean_p - a.mul(mean_I);

	//mean_a = boxfilter(a, r) ./ N;      
	cv::Mat mean_a;
	cv::boxFilter(a, mean_a, CV_32F, cv::Size(r, r));
	cv::Mat rmean_a;
	resize(mean_a, rmean_a, cv::Size(I_org.cols, I_org.rows), 1);

	//mean_b = boxfilter(b, r) ./ N;      
	cv::Mat mean_b;
	cv::boxFilter(b, mean_b, CV_32F, cv::Size(r, r));
	cv::Mat rmean_b;
	resize(mean_b, rmean_b, cv::Size(I_org.cols, I_org.rows), 1);

	//q = mean_a .* I + mean_b; % Eqn. (8) in the paper;      
	cv::Mat q = rmean_a.mul(_I) + rmean_b;

	return q;
}


cv::Mat GuidedFilter(cv::Mat I, cv::Mat p, int r, double eps)
{
	/*
	% GUIDEDFILTER   O(N) time implementation of guided filter.
	%
	%   - guidance image: I (should be a gray-scale/single channel image)
	%   - filtering input image: p (should be a gray-scale/single channel image)
	%   - local window radius: r
	%   - regularization parameter: eps
	*/

	cv::Mat _I;
	I.convertTo(_I, CV_32F);
	I = _I;

	cv::Mat _p;
	p.convertTo(_p, CV_32F);
	p = _p;

	//[hei, wid] = size(I);    
	int hei = I.rows;
	int wid = I.cols;

	r = 2 * r + 1;//��Ϊopencv�Դ���boxFilter�����е�Size,����9x9,����˵�뾶Ϊ4   

				  //mean_I = boxfilter(I, r) ./ N;    
	cv::Mat mean_I;
	cv::boxFilter(I, mean_I, CV_32F, cv::Size(r, r));

	//mean_p = boxfilter(p, r) ./ N;    
	cv::Mat mean_p;
	cv::boxFilter(p, mean_p, CV_32F, cv::Size(r, r));

	//mean_Ip = boxfilter(I.*p, r) ./ N;    
	cv::Mat mean_Ip;
	cv::boxFilter(I.mul(p), mean_Ip, CV_32F, cv::Size(r, r));

	//cov_Ip = mean_Ip - mean_I .* mean_p; % this is the covariance of (I, p) in each local patch.    
	cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p);

	//mean_II = boxfilter(I.*I, r) ./ N;    
	cv::Mat mean_II;
	cv::boxFilter(I.mul(I), mean_II, CV_32F, cv::Size(r, r));

	//var_I = mean_II - mean_I .* mean_I;    
	cv::Mat var_I = mean_II - mean_I.mul(mean_I);

	//a = cov_Ip ./ (var_I + eps); % Eqn. (5) in the paper;       
	cv::Mat a = cov_Ip / (var_I + eps);

	//b = mean_p - a .* mean_I; % Eqn. (6) in the paper;    
	cv::Mat b = mean_p - a.mul(mean_I);

	//mean_a = boxfilter(a, r) ./ N;    
	cv::Mat mean_a;
	cv::boxFilter(a, mean_a, CV_32F, cv::Size(r, r));

	//mean_b = boxfilter(b, r) ./ N;    
	cv::Mat mean_b;
	cv::boxFilter(b, mean_b, CV_32F, cv::Size(r, r));

	//q = mean_a .* I + mean_b; % Eqn. (8) in the paper;    
	cv::Mat q = mean_a.mul(I) + mean_b;

	return q;
}