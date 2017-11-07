
#define _CRT_SECURE_NO_WARNINGS

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <tesseract/strngs.h>

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;
using namespace std;
using namespace tesseract;

int main()
{
	// ���ʂ��e�L�X�g�ɏo��
	ofstream ofs("../image/component.txt");
	ofstream log("../image/log.txt");
	Mat src = imread("../../../source_images/img1_3_2.jpg",1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat line_map = src.clone();
	Mat word_map = src.clone();

	int para_row = 0;

	
	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //�i��
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL); //�s
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL); //�P��
	printf("Found textline = %d , word = %d .\n", line_boxes->n,word_boxes->n);
	ofs << "found textline = " << line_boxes->n << ",word = " << word_boxes->n << endl << endl;

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		api->SetRectangle(box->x, box->y, box->w, box->h);

		// �F�����ʂ��e�L�X�g�t�@�C���Ƃ��ďo�͂���
		//ofs << "Para_Box["<< i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;

		// ���������͈͂�؂����Ă��ꂼ��̉摜�ɂ���
		Rect rect(box->x, box->y, box->w, box->h);
		Mat part_img(src, rect);
		para_row = box->h;
		//imwrite("../image/splitImages/para_" + to_string(i) + ".png", part_img);

		// �摜�ɕ��������͈͂�`�ʂ���
		//rectangle(para_map, Point(box->x, box->y), Point(box->x+box->w, box->y+box->h),Scalar(0,0,255),1,4);
		//imwrite("../image/splitImages/map/para_map.png", para_map);
	}

	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		ofs << "Textline_Box[" << i << "]: x=" << box->x << "~" << box->x+box->w << ", y=" << box->y << "~" << box->y+box->h << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
		
		//Rect rect(box->x, box->y, box->w, box->h);
		//Mat part_img(src, rect);
		//imwrite("../image/splitImages/line_" + to_string(i) + ".png", part_img);

		//rectangle(line_map, Point(box->x, box->y), Point(box->x+box->w, box->y+box->h),Scalar(0,255,0),1,4);
		//imwrite("../image/splitImages/map/line_map.png", line_map);
		
	}

	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE); //para_boxes->n .. number of box in ptr array
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		ofs << "Word_Box[" << i << "]: x=" << box->x << "~" << box->x + box->w << ", y=" << box->y << "~" << box->y + box->h << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
	
		//Rect rect(box->x, box->y, box->w, box->h);
		//Mat part_img(src, rect);
		//imwrite("../image/splitImages/word_" + to_string(i) + ".png", part_img);

		//rectangle(word_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 0), 1, 4);
		//imwrite("../image/splitImages/map/word_map.png", word_map);
	}

	for (int i = 0; i < line_boxes->n; i++){
		int top = para_row;
		for (int j = 0; j < word_boxes->n; j++){
			BOX* line_box = boxaGetBox(line_boxes, i, L_CLONE); //1�s�̍��W (x,y,w,h)
			BOX* word_box = boxaGetBox(word_boxes, j, L_CLONE); //1�P��̍��W (x,y,w,h)
	
			for (int k = line_box->x; k <= line_box->x + line_box->w; k++){
				for (int m = line_box->y; m <= line_box->y + line_box->h; m++){
					if (k == word_box->x && m == word_box->y){
						log << "i=" << i << ", j=" << j << ", k=" << k << ", m=" << m << ", top=" << top << endl;

						// ��s�̒��ɂ���P��̒��ōł���ɂ��镨�̍��W�𓾂�
						if (m < top){
							top = m;
							//log << "top = m = " << m << endl;
						}
					}
				}
			}
			//��肾�����l���g���Ă̏����@��[�ɍ��킹��
		}
	}
}

