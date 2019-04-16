#ifndef LIB_BMP_H
#define LIB_BMP_H

#ifndef CAPI
#define CAPI extern
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(x) if (x) {free(x); x = NULL;}
#endif

#ifndef STRNULL
#define STRNULL(x) (x == NULL || strcmp(x, "") == 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMP
{
    unsigned char *data;
    int size, width, height;
    int alpha;
}BMP;

typedef struct BMPRect
{
    int left, right, top, bottom;
}BMPRect;

typedef struct BMPBGR
{
    int b, g, r;
}BMPBGR;

CAPI BMP *bmp_load(const char *file);

CAPI void bmp_save(BMP *bmp, const char *file);

CAPI void bmp_destroy(BMP **bmp);

// +---------------------------------------------------------
// | 图像处理 
// +---------------------------------------------------------

#ifndef PI
#define PI 3.1415926535897932384626
#endif

#ifndef BMPNULL
#define BMPNULL(bmp) (bmp == NULL || bmp->data == NULL || bmp->size == 0 || bmp->width == 0 || bmp->height == 0)
#endif

//一个扫描行实际所占字节数
#ifndef BMP_PERLINE_REALSIZE
#define BMP_PERLINE_REALSIZE(bmp) ((bmp->width * (bmp->alpha == 1 ? 32 : 24) + 31) / 32 * 4)
#endif

//一行扫描行补充的字节数
#ifndef BMP_PERLINE_SUP
#define BMP_PERLINE_SUP(bmp) (bmp->size / bmp->height - bmp->width * ((bmp->alpha == 1 ? 32 : 24) / 8))
#endif

#define BMP_LOOP_START(bmp, w, h) \
    for (h = 0; h < (int)bmp->height; h++) { \
        for (w = 0; w < (int)bmp->width; w++) {

#define BMP_LOOP_STOP(bmp, speed) \
            speed += bytepix; \
        } \
        speed += perline; \
    }

#define BMP_LOOP_STOP2(bmp, speed, bytepix, perline) \
        speed += bytepix; \
    } \
    speed += perline; \
}

/** 复制一个图像 **/
CAPI BMP *bmp_copy(BMP *bmp);

/** 复制图像中的一部分 **/
CAPI BMP *bmp_copy_rect(BMP *bmp, int left, int top, int right, int bottom);

/** 添加alpha通道 **/
CAPI void bmp_add_alpha(BMP *bmp);

/** 倒叙图像(摆正BMP) **/
CAPI void bmp_reverse(BMP *bmp);

/** 水平翻转 **/
CAPI void bmp_horizontal_flip(BMP *bmp);

/** 图像旋转 **/
/** flag == 1 自动改变图像大小 **/
CAPI void bmp_rotate(BMP *bmp, double angle, int flag, BMPBGR fillclr);

/** 指定颜色为背景色, 符合画布 **/
CAPI void bmp_resize_by_clr(BMP *bmp, BMPBGR bgclr);

/** 获得直方图数组 **/
CAPI int *bmp_histogram(BMP *bmp, int offset);

/** 灰度直方图 **/
CAPI int *bmp_grayhistogram(BMP *bmp);

/** 转灰度图 **/
CAPI void bmp_convert_gray(BMP *bmp);

/** 生成直方图 **/
CAPI BMP *bmp_create_histogram(int *histogram, BMPBGR clr);

/** 二值化 **/
CAPI void bmp_binaryzation(BMP *bmp, int k);

/** otsu算法 **/
CAPI int bmp_otsu(BMP *bmp);

/** 均值滤波 **/
CAPI void bmp_average_filter(BMP *bmp);

/** 方框滤波 **/
CAPI void bmp_box_filter(BMP *bmp, int box);

/** 中值滤波 **/
CAPI void bmp_middle_filter(BMP *bmp, int box);

/** 高斯滤波 **/
CAPI void bmp_gaussblur_filter(BMP *bmp, double sigma);

/** 矩阵卷积 **/
CAPI void bmp_convolution_filter(BMP *bmp, double **convolu, int size);

/** 对比bmp1, bmp2 **/
CAPI int bmp_contrast(BMP *bmp1, BMP *bmp2);

/** 在bmp中寻找bmp2 **/
CAPI int bmp_search(BMP *bmp, BMP *bmp2, int *x, int *y);

#ifdef __cplusplus
}
#endif

#endif
