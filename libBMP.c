#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "libBMP.h"

typedef unsigned short bmp_u_short;

#if (defined _M_X64) || (defined __x86_64__) || (defined __amd64)
typedef int bmp_u_long;
#else
typedef unsigned long bmp_u_long;
#endif

#pragma pack(1)
typedef struct
{  
    bmp_u_short bfType;
    bmp_u_long  bfSize;
    bmp_u_short bfReserved1;
    bmp_u_short bfReserved2;
    bmp_u_long  bfOffBits;
}BITMAP_FILE_HEADER;

typedef struct
{  
    bmp_u_long  biSize;
    bmp_u_long  biWidth;
    bmp_u_long  biHeight;
    bmp_u_short biPlanes;
    bmp_u_short biBitCount;
    bmp_u_long  biCompression;
    bmp_u_long  biSizeImage;
    bmp_u_long  biXPelsPerMeter;
    bmp_u_long  biYPelsPerMeter;
    bmp_u_long  biClrUsed;
    bmp_u_long  biClrImportant;
}BITMAP_INFO_HEADER;
#pragma pack()

BMP *bmp_load(const char *file)
{
    FILE *fp = NULL;
    BMP *bmp = NULL;
    BITMAP_FILE_HEADER file_header = {0};
    BITMAP_INFO_HEADER info_header = {0};
    
    if (STRNULL(file)) return NULL;

    fp = fopen(file, "rb");
    if (!fp) return NULL;

    if (fread(&file_header, sizeof(BITMAP_FILE_HEADER), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }
    
    if (fread(&info_header, sizeof(BITMAP_INFO_HEADER), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    if (file_header.bfType != 0x4d42) {
        fclose(fp);
        return NULL;
    }

    if ((bmp = (BMP *)malloc(sizeof(BMP))) == NULL) {
        fclose(fp);
        return NULL;
    }
    memset(bmp, 0, sizeof(BMP));

    //暂时只能支持24与32位
    if (info_header.biBitCount != 24 && info_header.biBitCount != 32) {
        free(bmp);
        fclose(fp);
        return NULL;
    }

    if (info_header.biBitCount == 32)
        bmp->alpha = 1;

    bmp->size = info_header.biSizeImage;
    bmp->width = info_header.biWidth;
    bmp->height = info_header.biHeight;

    if (bmp->size == 0) {
        bmp->size = BMP_PERLINE_REALSIZE(bmp) * bmp->height;
    }

    bmp->data = (unsigned char *)malloc(bmp->size);
    if (bmp->data == NULL) {
        free(bmp);
        fclose(fp);
        return NULL;
    }
    memset(bmp->data, 0, bmp->size);

    if (fread(bmp->data, 1, (size_t)bmp->size, fp) != (size_t)bmp->size) {
        free(bmp->data);
        free(bmp);
        fclose(fp);
        return NULL;
    }
    
    if (bmp->height < 0) {
        bmp->height = abs(bmp->height);
    } else {
        bmp_reverse(bmp);
    }

    fclose(fp);
    return bmp;
}

void bmp_save(BMP *bmp, const char *file)
{
    FILE *fp = NULL;
    BITMAP_FILE_HEADER kFileHeader;
    BITMAP_INFO_HEADER kInfoHeader;

    if (!bmp || STRNULL(file)) return;
    
    memset(&kFileHeader, 0, sizeof(BITMAP_FILE_HEADER));
    memset(&kInfoHeader, 0, sizeof(BITMAP_INFO_HEADER));

    kFileHeader.bfType = 0x4d42;
    kFileHeader.bfSize = sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER) + bmp->size;
    kFileHeader.bfReserved1 = 0;
    kFileHeader.bfReserved2 = 0;
    kFileHeader.bfOffBits = sizeof(BITMAP_FILE_HEADER) + sizeof(BITMAP_INFO_HEADER);
    
    kInfoHeader.biSize = sizeof(BITMAP_INFO_HEADER);
    kInfoHeader.biWidth = bmp->width;
    kInfoHeader.biHeight = bmp->height;
    kInfoHeader.biPlanes = 1;
    kInfoHeader.biBitCount = bmp->alpha == 1 ? 32 : 24;
    kInfoHeader.biCompression = 0L;
    kInfoHeader.biSizeImage = bmp->size;
    kInfoHeader.biXPelsPerMeter = 0;
    kInfoHeader.biYPelsPerMeter = 0;
    kInfoHeader.biClrUsed = 0;
    kInfoHeader.biClrImportant = 0;
    
    if ((fp = fopen(file, "wb+")) == NULL) return;
    fwrite(&kFileHeader, sizeof(BITMAP_FILE_HEADER), 1, fp);
    fwrite(&kInfoHeader, sizeof(BITMAP_INFO_HEADER), 1, fp);

    //倒转写入图像数据
    {
        unsigned char *src = NULL;
        int hsrc = 0, preline = 0;

        preline = BMP_PERLINE_REALSIZE(bmp);
        for (hsrc = bmp->height - 1; hsrc >= 0; hsrc--) {
            src = bmp->data + hsrc * preline;
            fwrite(src, preline, 1, fp);
        }
    }

    fclose(fp);
}

void bmp_destroy(BMP **bmp)
{
    if (bmp == NULL || *bmp == NULL)
        return;

    free((*bmp)->data);
    free(*bmp);
    *bmp = NULL;
}

// +---------------------------------------------------------
// | 图像处理
// +---------------------------------------------------------

/** 复制一个图像 **/
BMP *bmp_copy(BMP *bmp)
{
    BMP *dst = NULL;

    if (BMPNULL(bmp)) return NULL;

    dst = (BMP *)malloc(sizeof(BMP));
    if (dst == NULL) return NULL;

    dst->width = bmp->width;
    dst->height = bmp->height;
    dst->size = bmp->size;
    dst->alpha = bmp->alpha;
    dst->data = (unsigned char *)malloc(bmp->size);
    if (dst->data == NULL) {
        free(dst);
        return NULL;
    }

    memcpy(dst->data, bmp->data, dst->size);
    return dst;
}

/** 复制图像中的一部分 **/
BMP *bmp_copy_rect(BMP *bmp, int left, int top, int right, int bottom)
{
    BMP *dst = NULL;

    int bytepix = 0, perline = 0;
    int bytepix_dst = 0, perline_dst = 0;
    int w, h, dstw, dsth;
    int speed, speed_dst;
    
    if (BMPNULL(bmp)) return NULL;
    if (right <= left || bottom <= top) return NULL;
    
    dst = (BMP *)malloc(sizeof(BMP));
    if (dst == NULL) return NULL;

    right = right > bmp->width ? bmp->width : right;
    bottom = bottom > bmp->height ? bmp->height : bottom;

    dst->width = right - left;
    dst->height = bottom - top;
    dst->alpha = 0;
    dst->size = dst->height * BMP_PERLINE_REALSIZE(dst);

    bytepix = bmp->alpha == 1 ? 4 : 3;
    perline = BMP_PERLINE_REALSIZE(bmp);

    bytepix_dst = 3;
    perline_dst = BMP_PERLINE_REALSIZE(dst);

    dst->data = (unsigned char *)malloc(dst->size);
    if (dst->data == NULL) {
        free(dst);
        return NULL;
    }

    for (dsth = 0, h = top; h < (int)bottom; h++, dsth++) {
        for (dstw = 0, w = left; w < (int)right; w++, dstw++) {
            speed = h * perline + w * bytepix;
            speed_dst = dsth * perline_dst + dstw * bytepix_dst;


            dst->data[speed_dst] = bmp->data[speed];
            dst->data[speed_dst + 1] = bmp->data[speed + 1];
            dst->data[speed_dst + 2] = bmp->data[speed + 2];
        }
    }

    return dst;
}

/** 添加alpha通道 **/
void bmp_add_alpha(BMP *bmp)
{
    BMP buf = {0};
    
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    
    int w2 = 0, h2 = 0, speed2 = 0;
    int bytepix2 = 0, perline2 = 0;
    
    if (BMPNULL(bmp) || bmp->alpha == 1) return;
    
    buf = *bmp;
    
    bmp->alpha = 1;
    bmp->size = BMP_PERLINE_REALSIZE(bmp) * bmp->height;
    bmp->data = (unsigned char *)malloc(bmp->size);
    if (bmp->data == NULL) {
        *bmp = buf;
        return;
    }
    
    perline = (buf.size / buf.height - buf.width * (24 / 8));
    bytepix = 3;
    
    perline2 = (bmp->size / bmp->height - bmp->width * (32 / 8));
    bytepix2 = 4;
    
    for (h = 0, h2 = 0; h < (int)bmp->height; h++, h2++) {
        for (w = 0, w2 = 0; w < (int)bmp->width; w++, w2++) {
            bmp->data[speed2] = buf.data[speed];
            bmp->data[speed2 + 1] = buf.data[speed + 1];
            bmp->data[speed2 + 2] = buf.data[speed + 2];
            speed += bytepix;
            speed2 += bytepix2;
        }
        speed += perline;
        speed2 += perline2;
    }
    free(buf.data);
}

/** 倒转图像(摆正BMP) **/
void bmp_reverse(BMP *bmp)
{
    unsigned char *src = NULL, *dst = NULL, *new_data = NULL;
    int hsrc = 0, hdst = 0, preline = 0;
    
    if (BMPNULL(bmp)) return;

    new_data = (unsigned char *)malloc(bmp->size);
    if (new_data == NULL) return;

    preline = BMP_PERLINE_REALSIZE(bmp);
    for (hsrc = 0, hdst = bmp->height - 1; hsrc < (int)bmp->height; hsrc++, hdst--) {
        src = bmp->data + hsrc * preline;
        dst = new_data + hdst * preline;
        memcpy(dst, src, preline);
    }

    free(bmp->data);
    bmp->data = new_data;
}

/** 水平翻转 **/
void bmp_horizontal_flip(BMP *bmp)
{
    int i = 0, w = 0, h = 0, speedleft = 0, speedright = 0;
    int bytepix = 0, perline = 0;
    unsigned char left = 0, right = 0;

    if (BMPNULL(bmp)) return;

    perline = BMP_PERLINE_REALSIZE(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;

    for (h = 0; h < (int)bmp->height; h++) {
        for (w = 0; w < (int)bmp->width / 2; w++) {
            speedleft = h * perline + w * bytepix;
            speedright = h * perline + (bmp->width - 1 - w) * bytepix;
            for (i = 0; i < 3; i++) {
                left = bmp->data[speedleft + i];
                right = bmp->data[speedright + i];
                bmp->data[speedleft + i] = right;
                bmp->data[speedright + i] = left;
            }
        }
    }
}

/** 图像旋转 **/
/** flag == 1 自动改变图像大小 **/
void bmp_rotate(BMP *bmp, double angle, int flag, BMPBGR fillclr)
{
    double routeangle = 0.0f, cos_angle = 0.0f, sin_angle = 0.0f;
    int w = 0, h = 0, preline_real = 0, bytepix = 0, preline_real_dst = 0;
    int after_mid_x = 0, after_mid_y = 0, before_mid_x = 0, before_mid_y = 0;
    int after_x = 0, after_y = 0, before_x = 0, before_y = 0;
    int newwidth = 0, newheight = 0, newsize = 0;
    unsigned char *tmp = NULL;

    if (BMPNULL(bmp)) return;
    
    routeangle = 1.0 * angle * PI / 180;
    cos_angle = cos(routeangle);
    sin_angle = sin(routeangle);
    preline_real = BMP_PERLINE_REALSIZE(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;

    //不改变图像大小
    if (flag == 0) {
        before_mid_x = after_mid_x = bmp->width / 2;
        before_mid_y = after_mid_y = bmp->height / 2;
        newwidth = bmp->width;
        newheight = bmp->height;
        preline_real_dst = BMP_PERLINE_REALSIZE(bmp);
        newsize = preline_real_dst * newheight;
    } else {
        before_mid_x = bmp->width / 2;
        before_mid_y = bmp->height / 2;
        newwidth = bmp->width * 2;
        newheight = bmp->height * 2;
        after_mid_x = bmp->width;
        after_mid_y = bmp->height;
        preline_real_dst = ((newwidth * (bmp->alpha == 1 ? 32 : 24) + 31) / 32 * 4);
        newsize = preline_real_dst * newheight;
    }
    if ((tmp = (unsigned char *)malloc(newsize)) == NULL) return;

    //填充色
    for (h = 0; h < newheight; h++) {
        for (w = 0; w < newwidth; w++) {
            *(tmp + h * preline_real_dst + w * bytepix + 0) = fillclr.b;
            *(tmp + h * preline_real_dst + w * bytepix + 1) = fillclr.g;
            *(tmp + h * preline_real_dst + w * bytepix + 2) = fillclr.r;
        }
    }

    //旋转
    for (h = 0; h < newheight; h++) {
        for (w = 0; w < newwidth; w++) {
            after_y = h - after_mid_y;
            after_x = w - after_mid_x;
            before_y = (int)(cos_angle * after_y - sin_angle * after_x) + before_mid_y;
            before_x = (int)(sin_angle * after_y + cos_angle * after_x) + before_mid_x;
            if (before_y >= 0 && before_y < bmp->height && before_x >= 0 && before_x < bmp->width) {
                *(tmp + h * preline_real_dst + w * bytepix + 0) = *(bmp->data + before_y * preline_real + before_x * bytepix + 0);
                *(tmp + h * preline_real_dst + w * bytepix + 1) = *(bmp->data + before_y * preline_real + before_x * bytepix + 1);
                *(tmp + h * preline_real_dst + w * bytepix + 2) = *(bmp->data + before_y * preline_real + before_x * bytepix + 2);
            }
        }
    }

    free(bmp->data);
    bmp->width = newwidth;
    bmp->height = newheight;
    bmp->size = newsize;
    bmp->data = tmp;
}

/** 指定颜色为背景色, 符合画布 **/
void bmp_resize_by_clr(BMP *bmp, BMPBGR bgclr)
{
    int left = -1, top = -1, right = -1, bottom = -1;
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline_real = 0;
    BMP *dst = NULL;

    if (BMPNULL(bmp)) return;

    perline_real = BMP_PERLINE_REALSIZE(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;

    //上
    for (h = 0; h < (int)bmp->height; h++) {
        for (w = 0; w < (int)bmp->width; w++) {
            speed = h * perline_real + w * bytepix;
            if (bmp->data[speed] != bgclr.b || bmp->data[speed + 1] != bgclr.g || bmp->data[speed + 2] != bgclr.r) {
                top = h;
                break;
            }
        }
        if (top != -1) break;
    }

    //下
    for (h = (int)bmp->height - 1; h >= 0; h--) {
        for (w = 0; w < (int)bmp->width; w++) {
            speed = h * perline_real + w * bytepix;
            if (bmp->data[speed] != bgclr.b || bmp->data[speed + 1] != bgclr.g || bmp->data[speed + 2] != bgclr.r) {
                bottom = h;
                break;
            }
        }
        if (bottom != -1) break;
    }

    //左
    for (w = 0; w < (int)bmp->width; w++) {
        for (h = 0; h < (int)bmp->height; h++) {
            speed = h * perline_real + w * bytepix;
            if (bmp->data[speed] != bgclr.b || bmp->data[speed + 1] != bgclr.g || bmp->data[speed + 2] != bgclr.r) {
                left = w;
                break;
            }
        }
        if (left != -1) break;
    }

    //右
    for (w = (int)bmp->width - 1; w >= 0; w--) {
        for (h = 0; h < (int)bmp->height; h++) {
            speed = h * perline_real + w * bytepix;
            if (bmp->data[speed] != bgclr.b || bmp->data[speed + 1] != bgclr.g || bmp->data[speed + 2] != bgclr.r) {
                right = w;
                break;
            }
        }
        if (right != -1) break;
    }

    if (left != -1 && top != -1 && right != -1 && bottom != -1) {
        if (left <= right && top <= bottom && (dst = bmp_copy_rect(bmp, left, top, right, bottom)) != NULL) {
            bmp->width = dst->width;
            bmp->height = dst->height;
            bmp->size = dst->size;
            bmp->alpha = dst->alpha;
            free(bmp->data);
            bmp->data = dst->data;
            free(dst);
        }
    }
}

/** 获得直方图数组 **/
int *bmp_histogram(BMP *bmp, int offset)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    int *histogram = NULL;
    
    if (BMPNULL(bmp) || (histogram = (int *)malloc(sizeof(int) * 256)) == NULL)
        return NULL;
    memset(histogram, 0, sizeof(int) * 256);
    
    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);
    
    histogram[bmp->data[speed + offset]]++;
    
    BMP_LOOP_STOP(bmp, speed);
    return histogram;
}

/** 灰度直方图 **/
int *bmp_grayhistogram(BMP *bmp)
{
    bmp_convert_gray(bmp);
    return bmp_histogram(bmp, 0);
}

/** 生成直方图 **/
BMP *bmp_create_histogram(int *histogram, BMPBGR clr)
{
    BMP *bmp = NULL;

    int w = 0, h = 0, speed = 0;
    int bytepix = 4, perline_realsize = 0;
    int i = 0, max_height = 0;
    
    for (i = 0; i < 256; i++) {
        if (max_height < histogram[i])
            max_height = histogram[i];
    }
    if (max_height <= 0)
        max_height = 256;

    bmp = (BMP *)malloc(sizeof(BMP));
    if (!bmp) return NULL;

    bmp->width = 256;
    bmp->height = max_height;
    bmp->size = bmp->width * bmp->height * bytepix;
    bmp->alpha = 1;
    bmp->data = (unsigned char *)malloc(bmp->size);
    if (!bmp->data) {
        free(bmp);
        return NULL;
    }
    memset(bmp->data, 0, bmp->size);

    perline_realsize = BMP_PERLINE_REALSIZE(bmp);
    
    for (w = 0; w < 256; w++) {
        for (h = 0; h < histogram[w]; h++) {
            speed = h * perline_realsize + w *bytepix;
            bmp->data[speed + 0] = clr.b;
            bmp->data[speed + 1] = clr.g;
            bmp->data[speed + 2] = clr.r;
        }
    }

    bmp_reverse(bmp);
    return bmp;
}

/** 转灰度图 **/
void bmp_convert_gray(BMP *bmp)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;

    if (BMPNULL(bmp)) return;

    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);

    bmp->data[speed] = bmp->data[speed + 1] = bmp->data[speed + 2] = 
        (int)(bmp->data[speed + 2] * 0.3 + bmp->data[speed + 1] * 0.59 + bmp->data[speed] * 0.11);

    BMP_LOOP_STOP(bmp, speed);
}

/** 二值化 **/
void bmp_binaryzation(BMP *bmp, int k)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    int avg = 0;

    if (BMPNULL(bmp)) return;

    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);

    avg = (bmp->data[speed] + bmp->data[speed + 1] + bmp->data[speed + 2]) / 3;
    if (avg >= k) {
        bmp->data[speed] = bmp->data[speed + 1] = bmp->data[speed + 2] = 0xff;
    } else {
        bmp->data[speed] = bmp->data[speed + 1] = bmp->data[speed + 2] = 0x00;
    }

    BMP_LOOP_STOP(bmp, speed);
}

/** otsu算法 **/
int bmp_otsu(BMP *bmp)
{
    int threshold = 0, i = 0, j = 0, pixnum = 0, *grayhistogram = NULL;
    float hisportion[256] = {0};   //直方图比例
    BMP *buf = NULL;
    
    //占最大比例的像素
    float maxpoint = 0;
    int maxpoint_i = 0;
    
    float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
    
    if (BMPNULL(bmp) || (buf = bmp_copy(bmp)) == NULL) return 125;
    
    grayhistogram = bmp_grayhistogram(buf);
    pixnum = buf->width * buf->height;
    
    for (i = 0; i < 256; i++) {
        hisportion[i] = (float)grayhistogram[i] / pixnum;
        if (hisportion[i] > maxpoint) {
            maxpoint = hisportion[i];
            maxpoint_i = i;
        }
    }
    
    //遍历灰度级[0,255]
    for (i = 0; i < 256; i++) {    //i作为阈值
        w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;
        for (j = 0; j < 256; j++) {
            if (j <= i) {   //背景
                w0 += hisportion[j];
                u0tmp += j * hisportion[j];
            } else {        //前景
                w1 += hisportion[j];
                u1tmp += j * hisportion[j];
            }
        }
        u0 = u0tmp / w0;
        u1 = u1tmp / w1;
        u = u0tmp + u1tmp;
        deltaTmp = (float)(w0 * pow((u0 - u), 2) + w1 * pow((u1 - u), 2));
        if (deltaTmp > deltaMax) {
            deltaMax = deltaTmp;
            threshold = i;
        }
    }
    
    bmp_destroy(&buf);
    return threshold;
}

/** 计算均值/方框滤波 **/
static int bmp_average_filter_calc(BMP *bmp, int w, int h, int offset, int box)
{
    int x = 0, y = 0, avgvalue = 0, bufw = 0, bufh = 0;
    int bytepix = 0, perline = 0;

    perline = BMP_PERLINE_REALSIZE(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;

    for (x = -box; x <= box; x++) {
        for (y = -box; y <= box; y++) {
            bufw = (w + y) < 0 ? -(w + y) : (w + y);
            bufh = (h + x) < 0 ? -(h + x) : (h + x);
            bufw =  (bufw >= (int)bmp->width) ? bufw - (bufw - (int)bmp->width + 1) : bufw;
            bufh =  (bufh >= (int)bmp->height) ? bufh - (bufh - (int)bmp->height + 1) : bufh;
            avgvalue += bmp->data[bufh * perline + bufw * bytepix + offset];
        }
    }

    box = 2 * box + 1;
    return avgvalue / (box * box);
}

/** 均值滤波 **/
void bmp_average_filter(BMP *bmp)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    unsigned char *tmp = NULL;
    
    if (BMPNULL(bmp) || (tmp = (unsigned char *)malloc(bmp->size)) == NULL) return;
    
    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);
    
    tmp[speed]     = bmp_average_filter_calc(bmp, w, h, 0, 1);
    tmp[speed + 1] = bmp_average_filter_calc(bmp, w, h, 1, 1);
    tmp[speed + 2] = bmp_average_filter_calc(bmp, w, h, 2, 1);

    BMP_LOOP_STOP(bmp, speed);
    free(bmp->data);
    bmp->data = tmp;
}

/** 方框滤波 **/
void bmp_box_filter(BMP *bmp, int box)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    unsigned char *tmp = NULL;
    
    if (BMPNULL(bmp) || (tmp = (unsigned char *)malloc(bmp->size)) == NULL) return;
    
    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);

    tmp[speed]     = bmp_average_filter_calc(bmp, w, h, 0, box);
    tmp[speed + 1] = bmp_average_filter_calc(bmp, w, h, 1, box);
    tmp[speed + 2] = bmp_average_filter_calc(bmp, w, h, 2, box);
    
    BMP_LOOP_STOP(bmp, speed);
    free(bmp->data);
    bmp->data = tmp;
}

/** 快速排序算法 **/
static void quick_sort(int *s, int l, int r)
{
    if (l < r) {
        int i = l, j = r, x = s[l];
        while (i < j) {
            while (i < j && s[j] >= x) j--;
            if (i < j) s[i++] = s[j];
            
            while (i < j && s[i] < x) i++;
            if (i < j) s[j--] = s[i];
        }
        s[i] = x;
        quick_sort(s, l, i - 1);
        quick_sort(s, i + 1, r);
    }
}

/** 计算中值滤波 **/
static int bmp_middle_filter_calc(BMP *bmp, int w, int h, int offset, int box)
{
    int x = 0, y = 0, avgvalue = 0, bufw = 0, bufh = 0;
    int bytepix = 0, perline = 0;
    int *value = NULL, value_i = 0, box_size = 2 * box + 1, revalue = 0;
    
    box_size *= box_size;
    perline = BMP_PERLINE_REALSIZE(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;

    value = (int *)malloc(sizeof(int) * box_size);
    if (value == NULL)
        return bmp->data[h * perline + w * bytepix + offset];
    
    for (x = -box; x <= box; x++) {
        for (y = -box; y <= box; y++) {
            bufw = (w + y) < 0 ? -(w + y) : (w + y);
            bufh = (h + x) < 0 ? -(h + x) : (h + x);
            bufw =  (bufw >= (int)bmp->width) ? bufw - (bufw - (int)bmp->width + 1) : bufw;
            bufh =  (bufh >= (int)bmp->height) ? bufh - (bufh - (int)bmp->height + 1) : bufh;
            value[value_i++] = bmp->data[bufh * perline + bufw * bytepix + offset];
        }
    }
    
    quick_sort(value, 0, box_size - 1);
    revalue = value[box_size / 2];
    free(value);
    return revalue;
}

/** 中值滤波 **/
void bmp_middle_filter(BMP *bmp, int box)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    unsigned char *tmp = NULL;
    
    if (BMPNULL(bmp) || (tmp = (unsigned char *)malloc(bmp->size)) == NULL) return;
    
    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);

    tmp[speed]     = bmp_middle_filter_calc(bmp, w, h, 0, box);
    tmp[speed + 1] = bmp_middle_filter_calc(bmp, w, h, 1, box);
    tmp[speed + 2] = bmp_middle_filter_calc(bmp, w, h, 2, box);
    
    BMP_LOOP_STOP(bmp, speed);
    free(bmp->data);
    bmp->data = tmp;
}

/** 高斯公式 **/
static double gauss_twice(double sigma, double x, double y)
{
    return exp(-(x * x + y * y) / (2.0 * sigma * sigma)) / (sigma * sigma * 2 * PI);
}

/** 高斯矩阵 **/
static double **bmp_gaussblur(double sigma)
{
    int i = 0, j = 0;
    int winsize = (1 + (((int)ceil(3 * sigma)) * 2));
    int winsizehalf = winsize / 2;
    double **result = NULL;
    
    result = (double **)malloc(sizeof(double *) * winsize);
    for (i = 0; i < winsize; i++) {
        result[i] = (double *)malloc(sizeof(double) * winsize);
        for (j = 0; j < winsize; j++) {
            result[i][j] = gauss_twice(sigma, i - winsizehalf, j - winsizehalf);
        }
    }
    return result;
}

/** 高斯滤波 **/
void bmp_gaussblur_filter(BMP *bmp, double sigma)
{
    int i = 0, j = 0, winsize = 0;
    double **kern = NULL;

    if (BMPNULL(bmp) || (kern = bmp_gaussblur(sigma)) == NULL) return;

    winsize = (1 + (((int)ceil(3 * sigma)) * 2));
    bmp_convolution_filter(bmp, kern, winsize);

    for (i = 0; i < winsize; i++) {
        free(kern[i]);
    }
    free(kern);
}

/** 计算卷积 **/
static int bmp_convolution_filter_calc(BMP *bmp, int w, int h, int offset, double **convolu, int size)
{
    double value = 0.0f;

    int x = 0, y = 0, bufw = 0, bufh = 0, sizehalf = size / 2;
    int bytepix = 0, perline_realsize = 0;
    
    perline_realsize = BMP_PERLINE_REALSIZE(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    for (x = -sizehalf; x <= sizehalf; x++) {
        for (y = -sizehalf; y <= sizehalf; y++) {
            bufw = (w + y) < 0 ? -(w + y) : (w + y);
            bufh = (h + x) < 0 ? -(h + x) : (h + x);
            bufw =  (bufw >= (int)bmp->width) ? bufw - (bufw - (int)bmp->width + 1) : bufw;
            bufh =  (bufh >= (int)bmp->height) ? bufh - (bufh - (int)bmp->height + 1) : bufh;
            value += bmp->data[bufh * perline_realsize + bufw * bytepix + offset] * convolu[x + sizehalf][y + sizehalf];
        }
    }

    return (int)value;
}

/** 矩阵卷积 **/
void bmp_convolution_filter(BMP *bmp, double **convolu, int size)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;
    unsigned char *tmp = NULL;
    
    if (BMPNULL(bmp) || !convolu || (tmp = (unsigned char *)malloc(bmp->size)) == NULL) return;
    
    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(bmp, w, h);
    
    tmp[speed]     = bmp_convolution_filter_calc(bmp, w, h, 0, convolu, size);
    tmp[speed + 1] = bmp_convolution_filter_calc(bmp, w, h, 1, convolu, size);
    tmp[speed + 2] = bmp_convolution_filter_calc(bmp, w, h, 2, convolu, size);
    
    BMP_LOOP_STOP(bmp, speed);
    free(bmp->data);
    bmp->data = tmp;
}

/** 对比bmp1, bmp2 **/
int bmp_contrast(BMP *bmp1, BMP *bmp2)
{
    int w = 0, h = 0;
    int speed1 = 0, perline1 = 0, bytepix1 = 0;
    int speed2 = 0, perline2 = 0, bytepix2 = 0;

    if (BMPNULL(bmp1) || BMPNULL(bmp2)) return 0;
    if (bmp1->width != bmp2->width || bmp1->height != bmp2->height)
        return 0;

    perline1 = BMP_PERLINE_REALSIZE(bmp1);
    bytepix1 = bmp1->alpha == 1 ? 4 : 3;
    perline2 = BMP_PERLINE_REALSIZE(bmp2);
    bytepix2 = bmp2->alpha == 1 ? 4 : 3;

    for (h = 0; h < (int)bmp1->height; h++) {
        for (w = 0; w < (int)bmp1->width; w++) {
            speed1 = h * perline1 + w * bytepix1;
            speed2 = h * perline2 + w * bytepix2;
            
            if (bmp1->data[speed1] != bmp2->data[speed2] || 
                bmp1->data[speed1 + 1] != bmp2->data[speed2 + 1] ||
                bmp1->data[speed1 + 2] != bmp2->data[speed2 + 2])
                return 0;
        }
    }
    return 1;
}

static int bmp_search_ex(BMP *bmp, int w, int h, BMP *bmp2)
{
    BMP *bmp3 = NULL;
    int recode = 0;

    bmp3 = bmp_copy_rect(bmp, w, h, w + bmp2->width, h + bmp2->height);
    if (!bmp3) return 0;
    
    recode = bmp_contrast(bmp2, bmp3);
    bmp_destroy(&bmp3);
    return recode;
}

/** 在bmp中寻找bmp2 **/
int bmp_search(BMP *bmp, BMP *bmp2, int *x, int *y)
{
    int w = 0, h = 0;
    int w_size = 0, h_size = 0;   //bmp扫描范围

    if (BMPNULL(bmp) || BMPNULL(bmp2)) return 0;
    if (bmp->width < bmp2->width || bmp->height < bmp2->height) return 0;

    w_size = (int)bmp->width - (int)bmp2->width;
    h_size = (int)bmp->height - (int)bmp2->height;

    for (h = 0; h < h_size; h++) for (w = 0; w < w_size; w++) {
        if (bmp_search_ex(bmp, w, h, bmp2) == 1) {
            if (x) *x = w;
            if (y) *y = h;
            return 1;
        }
    }
    return 0;
}

/*
void function(BMP *bmp)
{
    int w = 0, h = 0, speed = 0;
    int bytepix = 0, perline = 0;

    if (BMPNULL(bmp)) return;

    perline = BMP_PERLINE_SUP(bmp);
    bytepix = bmp->alpha == 1 ? 4 : 3;
    BMP_LOOP_START(h, w, bmp);

    BMP_LOOP_STOP(speed, bmp);
}
*/
