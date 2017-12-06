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
	BOX* center_point = boxCreate(0,0,0,0);
	center_point->x = box->x + (box->w / 2);
	center_point->y = box->y + (box->h / 2);
	return center_point;
}

// 2つの単語の中心座標を取得し、そのベクトルを求める
BOX* getVector(BOX* box1, BOX* box2){
	BOX* vec = boxCreate(0, 0, 0, 0);
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
	ofstream vbox("../image/valid_boxes.txt");
	ofstream un_vbox("../image/un_visit_boxes.txt");

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
	for (int i = 0; i < 50; i++) { //**30
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 5 && box->w > 5){ // 閾値の調整が必要 **
			boxaAddBox(valid_boxes, box, L_CLONE);
			vbox << "box[" << i << "]= (" << box->x << "," << box->y << ")" << endl;
		}
	}

	// 先頭単語列
	Boxa* leading_boxes = boxaCreate(5);
	Box* box1 = boxCreate(15, 19, 106, 33);
	//Box* box2 = boxCreate(15, 61, 56, 27);
	//Box* box3 = boxCreate(14, 96, 134, 36);
	//Box* box4 = boxCreate(13, 138, 100, 28);
	boxaAddBox(leading_boxes, box1, L_CLONE);
	//boxaAddBox(leading_boxes, box2, L_CLONE);
	//boxaAddBox(leading_boxes, box3, L_CLONE);
	//boxaAddBox(leading_boxes, box4, L_CLONE);

	// 探索済みの単語列
	Boxa* searched_boxes = boxaCreate(800);

	// 未探索の単語列
	Boxa* unvisit_boxes = boxaCreate(800);
	set_difference(valid_boxes,leading_boxes,unvisit_boxes);
	for (int i = 0; i < unvisit_boxes->n; i++){
		BOX* box = boxaGetBox(unvisit_boxes, i, L_CLONE);
		un_vbox << "box[" << i << "]= (" << box->x << "," << box->y << ")" << endl;
	}


	BOX* st_point = boxCreate(0, 0, 0, 0);; //一つ目の単語
	BOX* ed_point = boxCreate(0, 0, 0, 0);; //二つ目の単語
	BOX* next_word = boxCreate(0, 0, 0, 0);; //次の単語
	BOX* vec;	// 2つの単語を結ぶベクトル
	BOX* base_vec = boxCreate(1, 0, 0, 0); // box2=(1,0) , X軸方向のベクトル
	double vec_size1; // ベクトルの大きさ1
	double vec_size2; // ベクトルの大きさ2
	double product; // 2つのベクトルの内積
	double vec_cos; // 2つのベクトルのなす角度

	double base_angle = cos(30.0 * PI / 180.0); // 次の単語を同じ行と判定する基準角度
	printf("base_angle=%f\n", base_angle);
	int min_x;
	int lbox_cnt = 0;

	// 初期値を設定
	st_point = boxaGetBox(leading_boxes, lbox_cnt, L_CLONE);
	printf("init : st_point=(%3d,%3d)\n", st_point->x, st_point->y);

	while (lbox_cnt < leading_boxes->n){
		min_x = 999;
		for (int j = 0; j < unvisit_boxes->n; j++){
			ed_point = boxaGetBox(unvisit_boxes, j, L_CLONE);
			printf("ed_point=(%3d,%3d)\n", ed_point->x, ed_point->y);
			// 2つの単語の中心座標を取得し、そのベクトルを求める
			vec = getVector(st_point, ed_point);

			// X軸方向のベクトルと上記で求めたベクトルのなす角度を求める
			// 各ベクトルの大きさを取得する
			vec_size1 = getVectorLength(vec);
			vec_size2 = getVectorLength(base_vec);

			// cosθを求める , cosθ=内積/ (√ベクトルの大きさ1*√ベクトルの大きさ2)
			vec_cos = getProduct(vec, base_vec) / (vec_size1 * vec_size2);
			printf("vec_cos=%f\n", vec_cos);

			// cosθが1/2(=30度から-30度)であれば右方向にあると判断する
			if (vec_cos >= base_angle){
				// 角度が30度から-30度の範囲にある、かつ最も近い(X座標が)単語を次の単語とする
				if (ed_point->x - st_point->x < min_x){
					min_x = ed_point->x - st_point->x;
					next_word = ed_point;
					printf("lbox_cnt=%3d , j=%3d , min_x=%3d\n", lbox_cnt,j,min_x);
				}
			}
		}
		// 上記の二つの条件を満たす単語を次の単語とする。そこから次の単語を探すため、同じ処理を行う。
		st_point = next_word;
		printf("Add : st_point=(%3d,%3d)\n", st_point->x, st_point->y);
		// st_pointを探索済みの配列に代入する
		boxaAddBox(searched_boxes, st_point, L_CLONE);
		// 右方向に単語が見つからなくなれば次の行へ移動する
		if (st_point == ed_point){
			printf("go to next row\n");
			lbox_cnt++;
		}
	}
}