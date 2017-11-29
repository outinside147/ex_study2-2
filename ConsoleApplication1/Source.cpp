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


// boxがboxesの中にあればtrue、そうでなければfalseを返す
bool set_member(BOX* box, Boxa* boxes){
	int i = 0;
	BOX* include_box = boxaGetBox(boxes, 0, L_CLONE);
	while (include_box != box && i != boxes->n - 1){
		i++;
		include_box = boxaGetBox(boxes, i, L_CLONE);
	}

	if (include_box == box){
		return true;
	}
	else {
		return false;
	}
}

// a_boxesの要素を一つずつ調べ、b_boxesの要素でないなら、c_boxesに加える
void set_difference(Boxa* a_boxes, Boxa* b_boxes, Boxa* c_boxes){
	int i, j;
	for (i = 0, j = 0; i < a_boxes->n; i++){
		BOX* include_abox = boxaGetBox(a_boxes, i, L_CLONE);
		if (!set_member(include_abox, b_boxes)){
			boxaAddBox(c_boxes, include_abox, L_CLONE);
		}
	}
}

int main()
{
	ofstream content("../image/component.txt");
	ofstream sortx("../image/sorted_x.txt");
	ofstream sorty("../image/sorted_y.txt");
	ofstream lboxes("../image/leading_boxes.txt");

	Mat src = imread("../../../source_images/img1_3_2.jpg", 1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat mat_para_img = imread("../image/splitImages/para.png", 1);

	int lparam_end = 0;
	int rparam_end = 0;

	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //段落

	// 文書画像から段落を抽出する
	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // 段落の左端
		rparam_end = box->x + box->w; //段落の右端
		// 分割した範囲を書き出す
		Rect rect(box->x, box->y, box->w, box->h);
		Mat part_img(src, rect);
		mat_para_img = part_img;
		imwrite("../image/splitImages/para.png", part_img);
		// 画像に分割した範囲を描写する
		/*
		rectangle(para_map, Point(box->x, box->y), Point(box->x+box->w, box->y+box->h),Scalar(0,0,255),1,4);
		imwrite("../image/splitImages/map_para.png", para_map);
		*/
	}

	// 抜き出した段落画像をTesseractで認識させる
	Mat pw_map = mat_para_img.clone();
	Pix *para_img = pixRead("../image/splitImages/para.png");
	TessBaseAPI *api2 = new TessBaseAPI();
	api2->Init(NULL, "eng");
	api2->SetImage(para_img);
	Boxa* word_boxes = api2->GetComponentImages(RIL_WORD, true, NULL, NULL); //単語
	for (int i = 0; i < word_boxes->n; i++){
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		/*
		rectangle(pw_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word.png", pw_map);
		*/
	}

	Mat word_map1 = mat_para_img.clone();
	Mat word_map2 = mat_para_img.clone();
	Mat word_map3 = mat_para_img.clone();

	// 明らかに誤認識している単語を消去する
	Boxa* valid_boxes = boxaCreate(word_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 5 && box->w > 5){ // 閾値の調整が必要 **
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
		/*
		Rect rect(box->x, box->y, box->w, box->h);
		Mat part_img(mat_para_img, rect);
		imwrite("../image/splitImages/word_" + to_string(i) + ".png", part_img);
		//
		rectangle(word_map1, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 0), 1, 4);
		imwrite("../image/splitImages/map_word_valid.png", word_map1);
		*/
	}

	Boxa* sort_xboxes = boxaCreate(500);
	Boxa* sort_yboxes = boxaCreate(100);
	Boxa* leading_boxes = boxaCreate(100);

	// xの昇順でソート
	sort_xboxes = boxaSort(valid_boxes, L_SORT_BY_X, L_SORT_INCREASING,NULL);
	for (int i = 0; i < sort_xboxes->n; i++){
		BOX* box = boxaGetBox(sort_xboxes, i, L_CLONE);
		//sortx << "sorted_x[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}

	// ソートした配列の先頭から100個の単語を行の先頭候補として別の配列に格納する
	for (int i = 0; i < 100; i++){
		BOX* box = boxaGetBox(sort_xboxes, i, L_CLONE);
		boxaAddBox(sort_yboxes, box, L_CLONE);
	}
	
	// yの昇順でソート
	sort_yboxes = boxaSort(sort_yboxes, L_SORT_BY_Y, L_SORT_INCREASING, NULL);
	for (int i = 0; i < sort_yboxes->n; i++){
		BOX* box = boxaGetBox(sort_yboxes, i, L_CLONE);
		/*
		rectangle(word_map2, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 255, 0), 1, 4);
		imwrite("../image/splitImages/map_word_ysort.png", word_map2);
		sorty << "sorted_y[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
		*/
		
	}

	int rcnt = 1;
	BOX* min_box = boxaGetBox(sort_yboxes, 0, L_CLONE);

	while (rcnt < sort_yboxes->n){
		BOX* cur_box = boxaGetBox(sort_yboxes, rcnt, L_CLONE);
		int dif_y = cur_box->y - min_box->y;
		if (dif_y >= 30){
			boxaAddBox(leading_boxes, min_box, L_CLONE);
			min_box = cur_box;
		}
		else if (dif_y < 30){
			if (cur_box->x < min_box->x){
				min_box = cur_box;
			}
		}
		rcnt++;
	}

	for (int i = 0; i < leading_boxes->n; i++){
		BOX* box = boxaGetBox(leading_boxes, i, L_CLONE);
		/*
		lboxes << "leading_boxes[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
		rectangle(word_map3, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word_leading.png", word_map3);
		*/
	}

}