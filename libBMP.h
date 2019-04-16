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
// | ͼ���� 
// +---------------------------------------------------------

#ifndef PI
#define PI 3.1415926535897932384626
#endif

#ifndef BMPNULL
#define BMPNULL(bmp) (bmp == NULL || bmp->data == NULL || bmp->size == 0 || bmp->width == 0 || bmp->height == 0)
#endif

//һ��ɨ����ʵ����ռ�ֽ���
#ifndef BMP_PERLINE_REALSIZE
#define BMP_PERLINE_REALSIZE(bmp) ((bmp->width * (bmp->alpha == 1 ? 32 : 24) + 31) / 32 * 4)
#endif

//һ��ɨ���в�����ֽ���
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

/** ����һ��ͼ�� **/
CAPI BMP *bmp_copy(BMP *bmp);

/** ����ͼ���е�һ���� **/
CAPI BMP *bmp_copy_rect(BMP *bmp, int left, int top, int right, int bottom);

/** ���alphaͨ�� **/
CAPI void bmp_add_alpha(BMP *bmp);

/** ����ͼ��(����BMP) **/
CAPI void bmp_reverse(BMP *bmp);

/** ˮƽ��ת **/
CAPI void bmp_horizontal_flip(BMP *bmp);

/** ͼ����ת **/
/** flag == 1 �Զ��ı�ͼ���С **/
CAPI void bmp_rotate(BMP *bmp, double angle, int flag, BMPBGR fillclr);

/** ָ����ɫΪ����ɫ, ���ϻ��� **/
CAPI void bmp_resize_by_clr(BMP *bmp, BMPBGR bgclr);

/** ���ֱ��ͼ���� **/
CAPI int *bmp_histogram(BMP *bmp, int offset);

/** �Ҷ�ֱ��ͼ **/
CAPI int *bmp_grayhistogram(BMP *bmp);

/** ת�Ҷ�ͼ **/
CAPI void bmp_convert_gray(BMP *bmp);

/** ����ֱ��ͼ **/
CAPI BMP *bmp_create_histogram(int *histogram, BMPBGR clr);

/** ��ֵ�� **/
CAPI void bmp_binaryzation(BMP *bmp, int k);

/** otsu�㷨 **/
CAPI int bmp_otsu(BMP *bmp);

/** ��ֵ�˲� **/
CAPI void bmp_average_filter(BMP *bmp);

/** �����˲� **/
CAPI void bmp_box_filter(BMP *bmp, int box);

/** ��ֵ�˲� **/
CAPI void bmp_middle_filter(BMP *bmp, int box);

/** ��˹�˲� **/
CAPI void bmp_gaussblur_filter(BMP *bmp, double sigma);

/** ������ **/
CAPI void bmp_convolution_filter(BMP *bmp, double **convolu, int size);

/** �Ա�bmp1, bmp2 **/
CAPI int bmp_contrast(BMP *bmp1, BMP *bmp2);

/** ��bmp��Ѱ��bmp2 **/
CAPI int bmp_search(BMP *bmp, BMP *bmp2, int *x, int *y);

#ifdef __cplusplus
}
#endif

#endif
