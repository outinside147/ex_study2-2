#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

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

#define PI  3.14159265358979323846

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

// 単語の中心座標を求める
BOX* getCenterPoint(BOX* box){
	BOX* center_point = nullptr;
	center_point->x = box->x + (box->w / 2);
	center_point->y = box->y + (box->h / 2);
	return center_point;
}

// 2つの単語の中心座標を取得し、そのベクトルを求める
BOX* getVector(BOX* box1, BOX* box2){
	BOX* vec = nullptr;
	BOX* center1 = getCenterPoint(box1);
	BOX* center2 = getCenterPoint(box2);
	vec->x = center1->x - center2->x;
	vec->y = center1->y - center2->y;
	return vec;
}

// ベクトルの大きさを取得する　返り値は√(x^2 + y^2)
double getVectorLength(BOX* box){
	return sqrt((box->x * box->x) + (box->y * box->y));
}

// 2つの単語の内積を求める
double getProduct(BOX* box1, BOX* box2){
	return (box1->x * box2->x) + (box1->y * box2->y);
}

int main()
{
	ofstream content("../image/component.txt");

	Mat src = imread("../../../source_images/img1_3_2.jpg", 1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat mat_para_img;

	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //段落

	// 文書画像から段落を抽出する
	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		// 分割した範囲を書き出す
		Rect rect(box->x, box->y, box->w, box->h);
		Mat part_img(src, rect);
		mat_para_img = part_img;
		imwrite("../image/splitImages/para.png", part_img);
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

	// 明らかに誤認識している単語を消去する
	Boxa* valid_boxes = boxaCreate(word_boxes->n);
	for (int i = 0; i < 30; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 5 && box->w > 5){ // 閾値の調整が必要 **
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}
	
	// 先頭単語列
	Boxa* leading_boxes = boxaCreate(5);
	Box* box1 = boxCreate(15, 19, 106, 33);
	Box* box2 = boxCreate(15, 61, 56, 27);
	Box* box3 = boxCreate(14, 96, 134, 36);
	Box* box4 = boxCreate(13, 138, 100, 28);
	boxaAddBox(leading_boxes, box1, L_CLONE);
	boxaAddBox(leading_boxes, box2, L_CLONE);
	boxaAddBox(leading_boxes, box3, L_CLONE);
	boxaAddBox(leading_boxes, box4, L_CLONE);

	BOX* st_point; //一つ目の単語
	BOX* ed_point; //二つ目の単語
	BOX* vec;	// 2つの単語を結ぶベクトル
	BOX* base_vec = boxCreate(1, 0, 0, 0); // box2=(1,0) , X軸方向のベクトル
	double vec_size1; // ベクトルの大きさ1
	double vec_size2; // ベクトルの大きさ2
	double product; // 2つのベクトルの内積
	double vec_cos; // 2つのベクトルのなす角度

	double base_angle = cos(30.0 * PI / 180.0); // 次の単語を同じ行と判定する基準角度
	int min_x = 999;
	int lbox_cnt = 1;
	int break_flg = 0;

	// 初期値を設定
	st_point = boxaGetBox(leading_boxes, 0, L_CLONE);

	while (lbox_cnt < leading_boxes->n){
		for (int j = 0; j < valid_boxes->n; j++){
			ed_point = boxaGetBox(valid_boxes, j, L_CLONE);
			// 2つの単語の中心座標を取得し、そのベクトルを求める
			vec = getVector(st_point, ed_point);

			// X軸方向のベクトルと上記で求めたベクトルのなす角度を求める
			// 各ベクトルの大きさを取得する
			vec_size1 = getVectorLength(vec);
			vec_size2 = getVectorLength(base_vec);

			// cosθを求める cosθ=内積/ (√ベクトルの大きさ1*√ベクトルの大きさ2)
			vec_cos = getProduct(vec, base_vec) / (vec_size1 * vec_size2);

			// cosθが1/2(=30度から-30度)であれば右方向にあると判断する
			if (vec_cos >= base_angle){
				// 角度が30度から-30度の範囲にある、かつ最も近い(X座標が)単語を次の単語とする
				if (ed_point->x-st_point->x < min_x){
					min_x = ed_point->x - st_point->x;
					break_flg = 1;
					break;
				}
			}
		}
		if (break_flg == 1){
			break_flg = 0;
			lbox_cnt++;
			break;
		}	
	}
}