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
	// 結果をテキストに出力
	ofstream ofs("../image/component.txt");

	Mat src = imread("../../../source_images/img1_3_2.jpg", 1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat line_map = src.clone();
	Mat word_map = src.clone();

	int lparam_end = 0;
	int rparam_end = 0;

	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //段落
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL); //行
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL); //単語
	printf("Found textline = %d , word = %d .\n", line_boxes->n, word_boxes->n);
	//ofs << "found textline = " << line_boxes->n << ",word = " << word_boxes->n << endl << endl;

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // 段落の左端
		rparam_end = box->x + box->w; //段落の右端

		api->SetRectangle(box->x, box->y, box->w, box->h);

		// 認識結果をテキストファイルとして出力する
		ofs << "Para_Box["<< i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
		
		// 分割した範囲を切り取ってそれぞれの画像にする
		//Rect rect(box->x, box->y, box->w, box->h);
		//Mat part_img(src, rect);
		//imwrite("../image/splitImages/para_" + to_string(i) + ".png", part_img);

		// 画像に分割した範囲を描写する
		//rectangle(para_map, Point(box->x, box->y), Point(box->x+box->w, box->y+box->h),Scalar(0,0,255),1,4);
		//imwrite("../image/splitImages/map_para.png", para_map);
	}

	// 行単位での認識結果を書き出す
	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);

		//rectangle(line_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 255, 0), 1, 4); //緑色
		//imwrite("../image/splitImages/map_line.png", line_map);
	}

	int min_x = src.cols;
	int min_y = src.rows;
	int min_w = 0;
	int min_h = 0;

	Boxa* correct_boxes = boxaCreate(50); // 文章の左上から調査して取得した順で単語を格納する配列 , 1行の単語列

	// 明らかに誤認識している単語を消去する
	Boxa* suit_Wboxes = boxaCreate(line_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 3  && box->w > 3){
			// 最も左上(最小)の座標を持つ単語領域を抽出する
			if (box->x < min_x && box->y < min_y){
				min_x = box->x;
				min_y = box->y;
				min_w = box->w;
				min_h = box->h;
			}
			boxaAddBox(suit_Wboxes, box, L_CLONE);
			//printf("word extracted : i=%3d , box->x=%3d , box->y=%3d , box->w=%3d , box->h=%3d .\n", i, box->x, box->y, box->w, box->h);
		}
	}

	printf("lparam_end=%3d, rparam_end=%3d\n", lparam_end, rparam_end);
	printf("init : min_x=%3d, min_y=%3d, min_w=%3d, min_h=%3d\n",min_x,min_y,min_w,min_h);

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < suit_Wboxes->n; i++) {
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		ofs << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;

		//Rect rect(box->x, box->y, box->w, box->h);
		//Mat part_img(src, rect);
		//imwrite("../image/splitImages/word_" + to_string(i) + ".png", part_img);

		//rectangle(word_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 0), 1, 4);
		//imwrite("../image/splitImages/map_word.png", word_map);
	}

	for (int i = 0; i < suit_Wboxes->n; i++) {
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		for (int j = min_x + min_w; j <= rparam_end; j++){
			for (int k = 0; k <= min_y + min_h; k++){
				if (j == box->x && k == box->y){
					boxaAddBox(correct_boxes, box, L_CLONE);
					if (i != suit_Wboxes->n){
						box = boxaGetBox(suit_Wboxes, i + 1, L_CLONE);
					}
					break;
				}
			}
		}
	}

	for (int i = 0; i < correct_boxes->n; i++) {
		BOX* box = boxaGetBox(correct_boxes, i, L_CLONE);
		printf("num=%2d, i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", correct_boxes->n, i, box->x, box->y, box->w, box->h);
	}
}