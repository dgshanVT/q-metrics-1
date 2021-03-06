#include <img.hpp>

/// @todo Talvez o uso de um template aqui seja mais sensato
void downsample(const cv::Mat & src,
                cv::Mat & dest,
                const int initRow,
                const int initCol,
                const int ratioRow,
                const int ratioCol)
{
    int i1 = 0, j1 = 0;
    assert(src.type() == dest.type());

    if(src.type() == CV_8UC1){
        for(int i = initRow; i < src.rows; i += ratioRow){
            j1 = 0;
            for(int j = initCol; j < src.cols; j += ratioCol){
                dest.at<uchar>(i1,j1) = src.at<uchar>(i,j);
                j1++;
            }
            i1++;
        }
    }
    if(src.type() == CV_64FC1){
        for(int i = initRow; i < src.rows; i += ratioRow){
            j1 = 0;
            for(int j = initCol; j < src.cols; j += ratioCol){
                dest.at<double>(i1,j1) = src.at<double>(i,j);
                j1++;
            }
            i1++;
        }
    }

    return ;
}

void FFT(const cv::Mat & src,cv::Mat & dest)
{
    assert((src.channels() == 1) && (dest.channels() == 2));

    int i,j;

    int height = src.rows;
    int width  = src.cols;

    fftw_complex * fft_src = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * height * width);
    fftw_plan p = fftw_plan_dft_2d(height,width,fft_src,fft_src,FFTW_FORWARD,FFTW_ESTIMATE);

    for(i = 0; i < height; ++i){
        for(j = 0; j < width; ++j){
            fft_src[j + i*width][0] = src.at<double>(i,j);
            fft_src[j + i*width][1] = 0.0;
        }
    }

    fftw_execute(p);

    for(i = 0; i < height; ++i){
        for(j = 0; j < width; ++j){
            dest.at<cv::Vec2d>(i,j)[0] = fft_src[j + i*width][0];
            dest.at<cv::Vec2d>(i,j)[1] = fft_src[j + i*width][1];
        }
    }

    fftw_destroy_plan(p);
    fftw_free(fft_src);
}


void IFFT(const cv::Mat & src,cv::Mat & dest)
{
    assert((src.channels() == 2) && (dest.channels() == 1));

    int i,j;

    int height = src.rows;
    int width  = src.cols;

    fftw_complex * ifft_src = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * height * width);
    fftw_plan p = fftw_plan_dft_2d(height,width,ifft_src,ifft_src,FFTW_BACKWARD,FFTW_ESTIMATE);

    for(i = 0; i < height; ++i){
        for(j = 0; j < width; ++j){
            ifft_src[j + i*width][0] = src.at<cv::Vec2f>(i,j)[0];
            ifft_src[j + i*width][1] = src.at<cv::Vec2f>(i,j)[1];
        }
    }

    fftw_execute(p);

    for(i = 0; i < height; ++i){
        for(j = 0; j < width; ++j){
            dest.at<float>(i,j) = ifft_src[j + i*width][0];
        }
    }

    fftw_destroy_plan(p);
    fftw_free(ifft_src);
}

double maxCorr2D(const cv::Mat & src1,const cv::Mat & src2)
{
    int i,j,k;
    double mag;

    int height = src1.rows;
    int width  = src1.cols;

    fftw_complex * img1 = (fftw_complex *) fftw_malloc(sizeof(fftw_complex)*height*width);
    fftw_complex * img2 = (fftw_complex *) fftw_malloc(sizeof(fftw_complex)*height*width);
    fftw_complex * res  = (fftw_complex *) fftw_malloc(sizeof(fftw_complex)*height*width);

    fftw_plan fft_img1 = fftw_plan_dft_1d(height*width,img1,img1,FFTW_FORWARD,FFTW_ESTIMATE);
    fftw_plan fft_img2 = fftw_plan_dft_1d(height*width,img2,img2,FFTW_FORWARD,FFTW_ESTIMATE);
    fftw_plan ifft_res = fftw_plan_dft_1d(height*width,res,res,FFTW_BACKWARD,FFTW_ESTIMATE);

    for(i = 0,k = 0; i < height; i++){
        for(j = 0; j < width; j++,k++){
            img1[k][0] = src1.at<double>(i,j);
            img1[k][1] = 0.0;

            img2[k][0] = src2.at<double>(i,j);
            img2[k][1] = 0.0;
        }
    }

    fftw_execute(fft_img1);
    fftw_execute(fft_img2);

    int fft_size = (height*width);

    for(i = 0; i < fft_size; ++i){
        res[i][0] = (img2[i][0] * img1[i][0]) - (img2[i][1] * (-img1[i][1]));
        res[i][1] = (img2[i][0] * (-img1[i][1])) + (img2[i][1] * img1[i][0]);

        mag = sqrt(pow(res[i][0],2.0) + pow(res[i][1],2.0));

        res[i][0] /= mag;
        res[i][1] /= mag;
    }

    fftw_execute(ifft_res);

    double max;
    max = -INFINITY;
    for(i = 0; i < fft_size; ++i){
        if( ((double)res[i][0]) > max)
            max = ((double)res[i][0]);
    }

    fftw_destroy_plan(fft_img1);
    fftw_destroy_plan(fft_img2);
    fftw_destroy_plan(ifft_res);
    fftw_free(img1);
    fftw_free(img2);
    fftw_free(res);

    return max;
}

void conv2D(const cv::Mat &img, cv::Mat& dest, const cv::Mat& kernel, ConvolutionType ctype, int btype) {

    cv::Mat source = img;
    cv::Mat flip_kernel(kernel.rows,kernel.cols,kernel.type());
    cv::flip(kernel,flip_kernel,-1);

    if(ctype == CONVOLUTION_FULL) {
        source = cv::Mat();
        const int additionalRows = kernel.rows-1, additionalCols = kernel.cols-1;
        cv::copyMakeBorder(img, source, (additionalRows+1)/2, additionalRows/2, (additionalCols+1)/2, additionalCols/2, btype, cv::Scalar(0));
    }

    cv::Point anchor(kernel.cols - kernel.cols/2 - 1, kernel.rows - kernel.rows/2 - 1);

    int borderMode = btype;
    cv::filter2D(source, dest, img.depth(), flip_kernel, anchor, 0, borderMode);

    if(ctype == CONVOLUTION_VALID) {
        dest = dest.colRange((kernel.cols-1)/2, dest.cols - kernel.cols/2)
                .rowRange((kernel.rows-1)/2, dest.rows - kernel.rows/2);
    }

}

void windowHamming(const cv::Mat & src,cv::Mat & dest)
{
    double ftmp = M_PI/(double)src.rows;
    for(int i = 0; i < src.rows; ++i){
        double wr = (0.54 + 0.46*cos(ftmp*((double)i+0.5)));
        for(int j = 0; j < src.cols; ++j){
            double wc = (0.54 + 0.46*cos(ftmp*((double)j+0.5)));

            dest.at<double>(i,j) = ((double)src.at<uchar>(i,j)) * wr * wc;
        }
    }
}

void filterLawsH(const cv::Mat &src,cv::Mat & dest,float r)
{
    cv::Mat source(src.rows,src.cols,CV_32FC1);
    src.assignTo(source,source.type());//Para casos em que o tipo de 'src' nao eh float

    conv2D(source,dest,(cv::Mat_<float>(5,5) <<
                     1/r,2/r ,0,-2/r ,-1/r,
                     4/r,8/r ,0,-8/r ,-4/r,
                     6/r,12/r,0,-12/r,-6/r,
                     4/r,8/r ,0,-8/r ,-4/r,
                     1/r,2/r ,0,-2/r ,-1/r));
    return ;
}


void filterLawsV(const cv::Mat & src,cv::Mat & dest,float r)
{
    cv::Mat source(src.rows,src.cols,CV_32FC1);
    src.assignTo(source,source.type());//Para casos em que o tipo de 'src' nao eh float

    conv2D(source,dest,(cv::Mat_<float>(5,5) <<
                     1/r, 4/r,  6/r, 4/r, 1/r,
                     2/r, 8/r, 12/r, 8/r, 2/r,
                     0  , 0  ,  0  , 0  , 0  ,
                     -2/r,-8/r,-12/r,-8/r,-2/r,
                     -1/r,-4/r, -6/r,-4/r,-1/r));
    return ;
}

void filterHantaoH(const cv::Mat & src,cv::Mat & dest)
{
    cv::Mat source(src.rows,src.cols,CV_32FC1);
    src.assignTo(source,source.type());//Para casos em que o tipo de 'src' nao eh float

    conv2D(source,dest,(cv::Mat_<float>(5,5) <<
                     1,1,0,1,1,
                     1,2,0,2,1,
                     1,2,0,2,1,
                     1,2,0,2,1,
                     1,1,0,1,1));
    return ;
}

void filterHantaoV(const cv::Mat & src,cv::Mat & dest)
{
    cv::Mat source(src.rows,src.cols,CV_32FC1);
    src.assignTo(source,source.type());//Para casos em que o tipo de 'src' nao eh float

    conv2D(source,dest,(cv::Mat_<float>(5,5) <<
                     1,1,1,1,1,
                     1,2,2,2,1,
                     0,0,0,0,0,
                     1,2,2,2,1,
                     1,1,1,1,1));
    return ;
}

/// Não é desejado utilizar essa funcao in-place - adequar um const no src
void analysisTexture(const cv::Mat & src,cv::Mat & dest)
{
    cv::Mat t1(src.rows,src.cols,src.type());
    cv::Mat t2(src.rows,src.cols,src.type());

    /// O padrão do OpenCV já faz padding simétrico (cv::BORDER_DEFAULT = cv::BORDER_REFLECT_101)
    filterLawsH(src,t1);
    filterLawsV(src,t2);

    t1 = cv::abs(t1);
    t2 = cv::abs(t2);

    for(int i = 0; i < src.rows; ++i)
        for(int j = 0; j < src.cols; ++j)
            if((t1.at<uchar>(i,j) > 0.15) || (t2.at<uchar>(i,j) > 0.15))
                dest.at<uchar>(i,j) = t1.at<uchar>(i+5,j+5) + t2.at<uchar>(i+5,j+5);

    return ;
}

void analysisContrast(const cv::Mat & src,cv::Mat & dest)
{
    cv::Scalar mean;
    cv::Scalar std;

    cv::Mat c1(src.rows,src.cols,src.type());
    cv::Mat c2(src.rows+33,src.cols+33,src.type());

    /// Filtra a imagem utilizando a funcao de sensitividade de contraste (CSF) no domínio espacial
    conv2D(src,c1,(cv::Mat_<float>(33,33) <<
                   4.1243e-08,3.2397e-08,1.1371e-08,-2.0774e-08,-6.0455e-08,-1.1145e-07,-1.8157e-07,-2.6422e-07,-3.301e-07,-3.4356e-07,-2.856e-07,-1.6191e-07,-6.9515e-10,1.4841e-07,2.343e-07,2.5095e-07,2.4572e-07,2.5095e-07,2.343e-07,1.4841e-07,-6.9515e-10,-1.6191e-07,-2.856e-07,-3.4356e-07,-3.301e-07,-2.6422e-07,-1.8157e-07,-1.1145e-07,-6.0455e-08,-2.0774e-08,1.1371e-08,3.2397e-08,4.1243e-08,
                   3.2397e-08,2.2705e-08,9.2313e-09,-9.8996e-09,-4.9927e-08,-1.1915e-07,-1.9687e-07,-2.4966e-07,-2.6984e-07,-2.7684e-07,-2.7219e-07,-2.0553e-07,1.1891e-08,4.6019e-07,1.1197e-06,1.766e-06,2.0425e-06,1.766e-06,1.1197e-06,4.6019e-07,1.1891e-08,-2.0553e-07,-2.7219e-07,-2.7684e-07,-2.6984e-07,-2.4966e-07,-1.9687e-07,-1.1915e-07,-4.9927e-08,-9.8996e-09,9.2313e-09,2.2705e-08,3.2397e-08,
                   1.1371e-08,9.2313e-09,7.1241e-09,-1.0239e-08,-5.5309e-08,-1.0363e-07,-1.2772e-07,-1.5906e-07,-2.5775e-07,-4.0892e-07,-4.9915e-07,-4.3263e-07,-2.7198e-07,-2.4771e-07,-5.5069e-07,-1.0417e-06,-1.2882e-06,-1.0417e-06,-5.5069e-07,-2.4771e-07,-2.7198e-07,-4.3263e-07,-4.9915e-07,-4.0892e-07,-2.5775e-07,-1.5906e-07,-1.2772e-07,-1.0363e-07,-5.5309e-08,-1.0239e-08,7.1241e-09,9.2313e-09,1.1371e-08,
                   -2.0774e-08,-9.8996e-09,-1.0239e-08,-3.2372e-08,-4.0323e-08,-1.2704e-08,-3.3299e-08,-2.0036e-07,-4.3824e-07,-5.3327e-07,-3.8192e-07,-7.8348e-08,3.1609e-07,9.8584e-07,2.1171e-06,3.3751e-06,3.9459e-06,3.3751e-06,2.1171e-06,9.8584e-07,3.1609e-07,-7.8348e-08,-3.8192e-07,-5.3327e-07,-4.3824e-07,-2.0036e-07,-3.3299e-08,-1.2704e-08,-4.0323e-08,-3.2372e-08,-1.0239e-08,-9.8996e-09,-2.0774e-08,
                   -6.0455e-08,-4.9927e-08,-5.5309e-08,-4.0323e-08,3.1377e-08,4.4257e-08,-1.2201e-07,-2.9598e-07,-2.008e-07,5.2816e-08,2.2358e-08,-4.6194e-07,-1.0754e-06,-1.5256e-06,-1.945e-06,-2.4459e-06,-2.7024e-06,-2.4459e-06,-1.945e-06,-1.5256e-06,-1.0754e-06,-4.6194e-07,2.2358e-08,5.2816e-08,-2.008e-07,-2.9598e-07,-1.2201e-07,4.4257e-08,3.1377e-08,-4.0323e-08,-5.5309e-08,-4.9927e-08,-6.0455e-08,
                   -1.1145e-07,-1.1915e-07,-1.0363e-07,-1.2704e-08,4.4257e-08,-8.2062e-08,-1.3261e-07,2.2502e-07,5.1969e-07,-1.0007e-07,-1.4557e-06,-2.2171e-06,-1.2838e-06,1.2671e-06,4.519e-06,7.2838e-06,8.3876e-06,7.2838e-06,4.519e-06,1.2671e-06,-1.2838e-06,-2.2171e-06,-1.4557e-06,-1.0007e-07,5.1969e-07,2.2502e-07,-1.3261e-07,-8.2062e-08,4.4257e-08,-1.2704e-08,-1.0363e-07,-1.1915e-07,-1.1145e-07,
                   -1.8157e-07,-1.9687e-07,-1.2772e-07,-3.3299e-08,-1.2201e-07,-1.3261e-07,4.0958e-07,7.6089e-07,-3.1224e-07,-1.9059e-06,-1.7349e-06,3.1369e-07,1.6616e-06,1.5685e-07,-3.5292e-06,-6.9366e-06,-8.2415e-06,-6.9366e-06,-3.5292e-06,1.5685e-07,1.6616e-06,3.1369e-07,-1.7349e-06,-1.9059e-06,-3.1224e-07,7.6089e-07,4.0958e-07,-1.3261e-07,-1.2201e-07,-3.3299e-08,-1.2772e-07,-1.9687e-07,-1.8157e-07,
                   -2.6422e-07,-2.4966e-07,-1.5906e-07,-2.0036e-07,-2.9598e-07,2.2502e-07,7.6089e-07,-3.6682e-07,-1.5158e-06,8.6586e-07,4.6284e-06,3.7091e-06,-2.1285e-06,-5.4078e-06,4.0555e-07,1.175e-05,1.7539e-05,1.175e-05,4.0555e-07,-5.4078e-06,-2.1285e-06,3.7091e-06,4.6284e-06,8.6586e-07,-1.5158e-06,-3.6682e-07,7.6089e-07,2.2502e-07,-2.9598e-07,-2.0036e-07,-1.5906e-07,-2.4966e-07,-2.6422e-07,
                   -3.301e-07,-2.6984e-07,-2.5775e-07,-4.3824e-07,-2.008e-07,5.1969e-07,-3.1224e-07,-1.5158e-06,2.2473e-06,6.8127e-06,4.3871e-07,-1.3554e-05,-1.9554e-05,-1.1949e-05,4.8718e-07,7.2008e-06,8.2578e-06,7.2008e-06,4.8718e-07,-1.1949e-05,-1.9554e-05,-1.3554e-05,4.3871e-07,6.8127e-06,2.2473e-06,-1.5158e-06,-3.1224e-07,5.1969e-07,-2.008e-07,-4.3824e-07,-2.5775e-07,-2.6984e-07,-3.301e-07,
                   -3.4356e-07,-2.7684e-07,-4.0892e-07,-5.3327e-07,5.2816e-08,-1.0007e-07,-1.9059e-06,8.6586e-07,6.8127e-06,-1.638e-06,-1.6904e-05,-5.9626e-06,2.2956e-05,3.319e-05,1.5353e-05,-1.1122e-05,-2.2544e-05,-1.1122e-05,1.5353e-05,3.319e-05,2.2956e-05,-5.9626e-06,-1.6904e-05,-1.638e-06,6.8127e-06,8.6586e-07,-1.9059e-06,-1.0007e-07,5.2816e-08,-5.3327e-07,-4.0892e-07,-2.7684e-07,-3.4356e-07,
                   -2.856e-07,-2.7219e-07,-4.9915e-07,-3.8192e-07,2.2358e-08,-1.4557e-06,-1.7349e-06,4.6284e-06,4.3871e-07,-1.6904e-05,8.1807e-06,6.1551e-05,3.4991e-05,-5.5282e-05,-0.00010312,-5.199e-05,5.4491e-06,-5.199e-05,-0.00010312,-5.5282e-05,3.4991e-05,6.1551e-05,8.1807e-06,-1.6904e-05,4.3871e-07,4.6284e-06,-1.7349e-06,-1.4557e-06,2.2358e-08,-3.8192e-07,-4.9915e-07,-2.7219e-07,-2.856e-07,
                   -1.6191e-07,-2.0553e-07,-4.3263e-07,-7.8348e-08,-4.6194e-07,-2.2171e-06,3.1369e-07,3.7091e-06,-1.3554e-05,-5.9626e-06,6.1551e-05,1.2939e-05,-0.00017577,-0.00022694,-0.00022987,-0.00011755,5.7546e-05,-0.00011755,-0.00022987,-0.00022694,-0.00017577,1.2939e-05,6.1551e-05,-5.9626e-06,-1.3554e-05,3.7091e-06,3.1369e-07,-2.2171e-06,-4.6194e-07,-7.8348e-08,-4.3263e-07,-2.0553e-07,-1.6191e-07,
                   -6.9515e-10,1.1891e-08,-2.7198e-07,3.1609e-07,-1.0754e-06,-1.2838e-06,1.6616e-06,-2.1285e-06,-1.9554e-05,2.2956e-05,3.4991e-05,-0.00017577,-0.00014129,0.00014561,-0.00092767,-0.0022122,-0.0022718,-0.0022122,-0.00092767,0.00014561,-0.00014129,-0.00017577,3.4991e-05,2.2956e-05,-1.9554e-05,-2.1285e-06,1.6616e-06,-1.2838e-06,-1.0754e-06,3.1609e-07,-2.7198e-07,1.1891e-08,-6.9515e-10,
                   1.4841e-07,4.6019e-07,-2.4771e-07,9.8584e-07,-1.5256e-06,1.2671e-06,1.5685e-07,-5.4078e-06,-1.1949e-05,3.319e-05,-5.5282e-05,-0.00022694,0.00014561,-0.00034885,-0.0043642,-0.00615,-0.0045863,-0.00615,-0.0043642,-0.00034885,0.00014561,-0.00022694,-5.5282e-05,3.319e-05,-1.1949e-05,-5.4078e-06,1.5685e-07,1.2671e-06,-1.5256e-06,9.8584e-07,-2.4771e-07,4.6019e-07,1.4841e-07,
                   2.343e-07,1.1197e-06,-5.5069e-07,2.1171e-06,-1.945e-06,4.519e-06,-3.5292e-06,4.0555e-07,4.8718e-07,1.5353e-05,-0.00010312,-0.00022987,-0.00092767,-0.0043642,-0.0079703,0.002361,0.014145,0.002361,-0.0079703,-0.0043642,-0.00092767,-0.00022987,-0.00010312,1.5353e-05,4.8718e-07,4.0555e-07,-3.5292e-06,4.519e-06,-1.945e-06,2.1171e-06,-5.5069e-07,1.1197e-06,2.343e-07,
                   2.5095e-07,1.766e-06,-1.0417e-06,3.3751e-06,-2.4459e-06,7.2838e-06,-6.9366e-06,1.175e-05,7.2008e-06,-1.1122e-05,-5.199e-05,-0.00011755,-0.0022122,-0.00615,0.002361,0.043915,0.076061,0.043915,0.002361,-0.00615,-0.0022122,-0.00011755,-5.199e-05,-1.1122e-05,7.2008e-06,1.175e-05,-6.9366e-06,7.2838e-06,-2.4459e-06,3.3751e-06,-1.0417e-06,1.766e-06,2.5095e-07,
                   2.4572e-07,2.0425e-06,-1.2882e-06,3.9459e-06,-2.7024e-06,8.3876e-06,-8.2415e-06,1.7539e-05,8.2578e-06,-2.2544e-05,5.4491e-06,5.7546e-05,-0.0022718,-0.0045863,0.014145,0.076061,0.12044,0.076061,0.014145,-0.0045863,-0.0022718,5.7546e-05,5.4491e-06,-2.2544e-05,8.2578e-06,1.7539e-05,-8.2415e-06,8.3876e-06,-2.7024e-06,3.9459e-06,-1.2882e-06,2.0425e-06,2.4572e-07,
                   2.5095e-07,1.766e-06,-1.0417e-06,3.3751e-06,-2.4459e-06,7.2838e-06,-6.9366e-06,1.175e-05,7.2008e-06,-1.1122e-05,-5.199e-05,-0.00011755,-0.0022122,-0.00615,0.002361,0.043915,0.076061,0.043915,0.002361,-0.00615,-0.0022122,-0.00011755,-5.199e-05,-1.1122e-05,7.2008e-06,1.175e-05,-6.9366e-06,7.2838e-06,-2.4459e-06,3.3751e-06,-1.0417e-06,1.766e-06,2.5095e-07,
                   2.343e-07,1.1197e-06,-5.5069e-07,2.1171e-06,-1.945e-06,4.519e-06,-3.5292e-06,4.0555e-07,4.8718e-07,1.5353e-05,-0.00010312,-0.00022987,-0.00092767,-0.0043642,-0.0079703,0.002361,0.014145,0.002361,-0.0079703,-0.0043642,-0.00092767,-0.00022987,-0.00010312,1.5353e-05,4.8718e-07,4.0555e-07,-3.5292e-06,4.519e-06,-1.945e-06,2.1171e-06,-5.5069e-07,1.1197e-06,2.343e-07,
                   1.4841e-07,4.6019e-07,-2.4771e-07,9.8584e-07,-1.5256e-06,1.2671e-06,1.5685e-07,-5.4078e-06,-1.1949e-05,3.319e-05,-5.5282e-05,-0.00022694,0.00014561,-0.00034885,-0.0043642,-0.00615,-0.0045863,-0.00615,-0.0043642,-0.00034885,0.00014561,-0.00022694,-5.5282e-05,3.319e-05,-1.1949e-05,-5.4078e-06,1.5685e-07,1.2671e-06,-1.5256e-06,9.8584e-07,-2.4771e-07,4.6019e-07,1.4841e-07,
                   -6.9515e-10,1.1891e-08,-2.7198e-07,3.1609e-07,-1.0754e-06,-1.2838e-06,1.6616e-06,-2.1285e-06,-1.9554e-05,2.2956e-05,3.4991e-05,-0.00017577,-0.00014129,0.00014561,-0.00092767,-0.0022122,-0.0022718,-0.0022122,-0.00092767,0.00014561,-0.00014129,-0.00017577,3.4991e-05,2.2956e-05,-1.9554e-05,-2.1285e-06,1.6616e-06,-1.2838e-06,-1.0754e-06,3.1609e-07,-2.7198e-07,1.1891e-08,-6.9515e-10,
                   -1.6191e-07,-2.0553e-07,-4.3263e-07,-7.8348e-08,-4.6194e-07,-2.2171e-06,3.1369e-07,3.7091e-06,-1.3554e-05,-5.9626e-06,6.1551e-05,1.2939e-05,-0.00017577,-0.00022694,-0.00022987,-0.00011755,5.7546e-05,-0.00011755,-0.00022987,-0.00022694,-0.00017577,1.2939e-05,6.1551e-05,-5.9626e-06,-1.3554e-05,3.7091e-06,3.1369e-07,-2.2171e-06,-4.6194e-07,-7.8348e-08,-4.3263e-07,-2.0553e-07,-1.6191e-07,
                   -2.856e-07,-2.7219e-07,-4.9915e-07,-3.8192e-07,2.2358e-08,-1.4557e-06,-1.7349e-06,4.6284e-06,4.3871e-07,-1.6904e-05,8.1807e-06,6.1551e-05,3.4991e-05,-5.5282e-05,-0.00010312,-5.199e-05,5.4491e-06,-5.199e-05,-0.00010312,-5.5282e-05,3.4991e-05,6.1551e-05,8.1807e-06,-1.6904e-05,4.3871e-07,4.6284e-06,-1.7349e-06,-1.4557e-06,2.2358e-08,-3.8192e-07,-4.9915e-07,-2.7219e-07,-2.856e-07,
                   -3.4356e-07,-2.7684e-07,-4.0892e-07,-5.3327e-07,5.2816e-08,-1.0007e-07,-1.9059e-06,8.6586e-07,6.8127e-06,-1.638e-06,-1.6904e-05,-5.9626e-06,2.2956e-05,3.319e-05,1.5353e-05,-1.1122e-05,-2.2544e-05,-1.1122e-05,1.5353e-05,3.319e-05,2.2956e-05,-5.9626e-06,-1.6904e-05,-1.638e-06,6.8127e-06,8.6586e-07,-1.9059e-06,-1.0007e-07,5.2816e-08,-5.3327e-07,-4.0892e-07,-2.7684e-07,-3.4356e-07,
                   -3.301e-07,-2.6984e-07,-2.5775e-07,-4.3824e-07,-2.008e-07,5.1969e-07,-3.1224e-07,-1.5158e-06,2.2473e-06,6.8127e-06,4.3871e-07,-1.3554e-05,-1.9554e-05,-1.1949e-05,4.8718e-07,7.2008e-06,8.2578e-06,7.2008e-06,4.8718e-07,-1.1949e-05,-1.9554e-05,-1.3554e-05,4.3871e-07,6.8127e-06,2.2473e-06,-1.5158e-06,-3.1224e-07,5.1969e-07,-2.008e-07,-4.3824e-07,-2.5775e-07,-2.6984e-07,-3.301e-07,
                   -2.6422e-07,-2.4966e-07,-1.5906e-07,-2.0036e-07,-2.9598e-07,2.2502e-07,7.6089e-07,-3.6682e-07,-1.5158e-06,8.6586e-07,4.6284e-06,3.7091e-06,-2.1285e-06,-5.4078e-06,4.0555e-07,1.175e-05,1.7539e-05,1.175e-05,4.0555e-07,-5.4078e-06,-2.1285e-06,3.7091e-06,4.6284e-06,8.6586e-07,-1.5158e-06,-3.6682e-07,7.6089e-07,2.2502e-07,-2.9598e-07,-2.0036e-07,-1.5906e-07,-2.4966e-07,-2.6422e-07,
                   -1.8157e-07,-1.9687e-07,-1.2772e-07,-3.3299e-08,-1.2201e-07,-1.3261e-07,4.0958e-07,7.6089e-07,-3.1224e-07,-1.9059e-06,-1.7349e-06,3.1369e-07,1.6616e-06,1.5685e-07,-3.5292e-06,-6.9366e-06,-8.2415e-06,-6.9366e-06,-3.5292e-06,1.5685e-07,1.6616e-06,3.1369e-07,-1.7349e-06,-1.9059e-06,-3.1224e-07,7.6089e-07,4.0958e-07,-1.3261e-07,-1.2201e-07,-3.3299e-08,-1.2772e-07,-1.9687e-07,-1.8157e-07,
                   -1.1145e-07,-1.1915e-07,-1.0363e-07,-1.2704e-08,4.4257e-08,-8.2062e-08,-1.3261e-07,2.2502e-07,5.1969e-07,-1.0007e-07,-1.4557e-06,-2.2171e-06,-1.2838e-06,1.2671e-06,4.519e-06,7.2838e-06,8.3876e-06,7.2838e-06,4.519e-06,1.2671e-06,-1.2838e-06,-2.2171e-06,-1.4557e-06,-1.0007e-07,5.1969e-07,2.2502e-07,-1.3261e-07,-8.2062e-08,4.4257e-08,-1.2704e-08,-1.0363e-07,-1.1915e-07,-1.1145e-07,
                   -6.0455e-08,-4.9927e-08,-5.5309e-08,-4.0323e-08,3.1377e-08,4.4257e-08,-1.2201e-07,-2.9598e-07,-2.008e-07,5.2816e-08,2.2358e-08,-4.6194e-07,-1.0754e-06,-1.5256e-06,-1.945e-06,-2.4459e-06,-2.7024e-06,-2.4459e-06,-1.945e-06,-1.5256e-06,-1.0754e-06,-4.6194e-07,2.2358e-08,5.2816e-08,-2.008e-07,-2.9598e-07,-1.2201e-07,4.4257e-08,3.1377e-08,-4.0323e-08,-5.5309e-08,-4.9927e-08,-6.0455e-08,
                   -2.0774e-08,-9.8996e-09,-1.0239e-08,-3.2372e-08,-4.0323e-08,-1.2704e-08,-3.3299e-08,-2.0036e-07,-4.3824e-07,-5.3327e-07,-3.8192e-07,-7.8348e-08,3.1609e-07,9.8584e-07,2.1171e-06,3.3751e-06,3.9459e-06,3.3751e-06,2.1171e-06,9.8584e-07,3.1609e-07,-7.8348e-08,-3.8192e-07,-5.3327e-07,-4.3824e-07,-2.0036e-07,-3.3299e-08,-1.2704e-08,-4.0323e-08,-3.2372e-08,-1.0239e-08,-9.8996e-09,-2.0774e-08,
                   1.1371e-08,9.2313e-09,7.1241e-09,-1.0239e-08,-5.5309e-08,-1.0363e-07,-1.2772e-07,-1.5906e-07,-2.5775e-07,-4.0892e-07,-4.9915e-07,-4.3263e-07,-2.7198e-07,-2.4771e-07,-5.5069e-07,-1.0417e-06,-1.2882e-06,-1.0417e-06,-5.5069e-07,-2.4771e-07,-2.7198e-07,-4.3263e-07,-4.9915e-07,-4.0892e-07,-2.5775e-07,-1.5906e-07,-1.2772e-07,-1.0363e-07,-5.5309e-08,-1.0239e-08,7.1241e-09,9.2313e-09,1.1371e-08,
                   3.2397e-08,2.2705e-08,9.2313e-09,-9.8996e-09,-4.9927e-08,-1.1915e-07,-1.9687e-07,-2.4966e-07,-2.6984e-07,-2.7684e-07,-2.7219e-07,-2.0553e-07,1.1891e-08,4.6019e-07,1.1197e-06,1.766e-06,2.0425e-06,1.766e-06,1.1197e-06,4.6019e-07,1.1891e-08,-2.0553e-07,-2.7219e-07,-2.7684e-07,-2.6984e-07,-2.4966e-07,-1.9687e-07,-1.1915e-07,-4.9927e-08,-9.8996e-09,9.2313e-09,2.2705e-08,3.2397e-08,
                   4.1243e-08,3.2397e-08,1.1371e-08,-2.0774e-08,-6.0455e-08,-1.1145e-07,-1.8157e-07,-2.6422e-07,-3.301e-07,-3.4356e-07,-2.856e-07,-1.6191e-07,-6.9515e-10,1.4841e-07,2.343e-07,2.5095e-07,2.4572e-07,2.5095e-07,2.343e-07,1.4841e-07,-6.9515e-10,-1.6191e-07,-2.856e-07,-3.4356e-07,-3.301e-07,-2.6422e-07,-1.8157e-07,-1.1145e-07,-6.0455e-08,-2.0774e-08,1.1371e-08,3.2397e-08,4.1243e-08));

    cv::copyMakeBorder(c1,c2,33,33,33,33,cv::BORDER_DEFAULT);

    for(int i = 0; i < src.rows; i++)
    {
        for(int j = 0; j < src.cols; j++)
        {
            cv::Mat block_ij(c2, cv::Rect(i, j, 67, 67)); /* Tamanho do bloco: 33 + 33 + 1 */
            cv::meanStdDev(block_ij,mean,std);

            dest.at<float>(i,j) = std[0]/mean[0];
        }
    }

    return ;
}

void localContrastRMS( const cv::Mat &src,cv::Mat &dest, const int stdev)
{
    double std_deviation, mean;
    cv::Mat mean_local(src.rows,src.cols,CV_32FC1);
    cv::Mat mean_sq(src.rows,src.cols,CV_32FC1);
    cv::Mat source(src.rows,src.cols,CV_32FC1);
    src.assignTo(source,source.type());//Para casos em que o tipo de 'src' nao eh float

    mean_sq = source.mul(source);//Gera uma matriz com os valores de src ao quadrado

    cv::GaussianBlur(source,mean_local,cv::Size(stdev,stdev),0,0);
    cv::GaussianBlur(mean_sq,mean_sq,cv::Size(stdev,stdev),0,0);

    for(int i = 0;i < src.rows; i++){
        for(int j = 0; j < src.cols; j++){
            mean = mean_local.at<float>(i,j);

            std_deviation = sqrt(mean_sq.at<float>(i,j) - (mean * mean));
            dest.at<float>(i,j) = std_deviation / mean;
        }
    }

    return ;
}

void filterRank(const cv::Mat &src,cv::Mat & dest)
{
    int i,j;

    assert((dest.rows == (src.rows - 1)) && (dest.cols == (src.cols - 1)));

    /// Buffer para guardar o filtro vertical
    cv::Mat temp(src.rows - 1, src.cols, src.type());

    /// Operador de diferença na direção vertical
    for(i = 0; i < src.rows - 1; ++i){
        for(j = 0; j < src.cols; ++j){
            temp.at<uchar>(i,j) = src.at<uchar>(i+1,j) - src.at<uchar>(i,j);
        }
    }
    /// Operador de diferença na direção horizontal
    for(i = 0; i < src.rows - 1; ++i){
        for(j = 0; j < src.cols - 1; ++j){
            dest.at<uchar>(i,j) = temp.at<uchar>(i,j+1) - temp.at<uchar>(i,j);
        }
    }

    return ;
}



