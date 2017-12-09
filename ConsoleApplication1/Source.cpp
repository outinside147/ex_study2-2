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

typedef struct { double x, y;} Box_array;

// boxがboxesの中にあればtrue、そうでなければfalseを返す
bool set_member(BOX* box, Boxa* boxes){
	int i = 0;
	BOX* include_box = boxaGetBox(boxes, 0, L_CLONE);
	while (include_box != box && i != boxes->n - 1){
		i++;
		include_box = boxaGetBox(boxes, i, L_CLONE);
	}
	printf("set_member : include_box = (%3d,%3d)\n", include_box->x, include_box->y);

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
		printf("set_difference : include_abox = (%3d,%3d)\n", include_abox->x, include_abox->y);
		if (!set_member(include_abox, b_boxes)){
			boxaAddBox(c_boxes, include_abox, L_CLONE);
		}
	}
}

// 単語の中心座標を求める
Box_array getCenterPoint(BOX* box){
	Box_array center_point = { 0, 0 };
	center_point.x = box->x + (box->w / 2);
	center_point.y = box->y - (box->h / 2);
	return center_point;
}

// 2つの単語の中心座標を取得し、そのベクトルを求める
Box_array getVector(BOX* box1, BOX* box2){
	Box_array vec = {0,0};
	Box_array st_center = getCenterPoint(box1);
	Box_array ed_center = getCenterPoint(box2);
	vec.x = ed_center.x - st_center.x;
	vec.y = ed_center.y - st_center.y;
	return vec;
}

// ベクトルの大きさを取得する　返り値は√(x^2 + y^2)
double getVectorLength(Box_array box){
	return sqrt((box.x * box.x) + (box.y * box.y));
}

// 2つの単語の内積を求める
double getProduct(Box_array box, Box_array base_box){
	return (box.x * base_box.x) + (box.y * base_box.y);
}

Box* create_box(int x, int y, int w, int h){
	BOX* box = boxCreate(x, y, w, h);
	return box;
}

int main()
{
	// 先頭単語列
	Boxa* leading_boxes = boxaCreate(50);
	BOX* lbox1 = create_box(0, 0, 2, 2);
	boxaAddBox(leading_boxes, lbox1, L_CLONE);

	Boxa* valid_boxes = boxaCreate(50);
	BOX* vbox1 = create_box(0, 0, 2, 2);
	BOX* vbox2 = create_box(2, 1, 2, 2);
	BOX* vbox3 = create_box(2, 2, 2, 2);
	BOX* vbox4 = create_box(2, 3, 2, 2);
	BOX* vbox5 = create_box(4, 2, 2, 2);
	BOX* vbox6 = create_box(4, 3, 2, 2);
	BOX* vbox7 = create_box(6, 0, 2, 2);
	BOX* vbox8 = create_box(6, 1, 2, 2);
	BOX* vbox9 = create_box(6, 4, 2, 2);
	boxaAddBox(valid_boxes, vbox1, L_CLONE);
	boxaAddBox(valid_boxes, vbox2, L_CLONE);
	boxaAddBox(valid_boxes, vbox3, L_CLONE);
	boxaAddBox(valid_boxes, vbox4, L_CLONE);
	boxaAddBox(valid_boxes, vbox5, L_CLONE);
	boxaAddBox(valid_boxes, vbox6, L_CLONE);
	boxaAddBox(valid_boxes, vbox7, L_CLONE);
	boxaAddBox(valid_boxes, vbox8, L_CLONE);
	boxaAddBox(valid_boxes, vbox9, L_CLONE);

	// 探索済みの単語列
	Boxa* searched_boxes = boxaCreate(100);

	for (int i = 0; i < valid_boxes->n; i++){
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		printf("box[%d]=(%3d,%3d)\n", i, box->x, box->y);
	}

	BOX* st_point = boxCreate(0, 0, 0, 0);; //一つ目の単語
	BOX* ed_point = boxCreate(0, 0, 0, 0);; //二つ目の単語
	BOX* next_word = boxCreate(0, 0, 0, 0);; //次の単語
	Box_array vec = { 0, 0 };	// 2つの単語を結ぶベクトル
	Box_array base_vec = { 1, 0 }; // box2=(1,0) , X軸方向のベクトル
	double vec_size1; // ベクトルの大きさ1
	double vec_size2; // ベクトルの大きさ2
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
		for (int j = 0; j < valid_boxes->n; j++){
			ed_point = boxaGetBox(valid_boxes, j, L_CLONE);
			//printf("st_point=(%3d,%3d),ed_point=(%3d,%3d)\n", st_point->x, st_point->y, ed_point->x, ed_point->y);
			// 2つの単語の中心座標を取得し、そのベクトルを求める
			vec = getVector(st_point, ed_point);

			// X軸方向のベクトルと上記で求めたベクトルのなす角度を求める
			// 各ベクトルの大きさを取得する
			vec_size1 = getVectorLength(vec);
			vec_size2 = getVectorLength(base_vec);

			// cosθを求める , cosθ=内積/ (√ベクトルの大きさ1*√ベクトルの大きさ2)
			vec_cos = getProduct(vec, base_vec) / (vec_size1 * vec_size2);
			//printf("vec_cos=%f\n", vec_cos);

			// cosθが1/2(=30度から-30度)であれば右方向にあると判断する
			if (vec_cos >= base_angle){
				// 角度が30度から-30度の範囲にある、かつ最も近い(X座標が)単語を次の単語とする
				if (ed_point->x - st_point->x < min_x){
					min_x = ed_point->x - st_point->x;
					next_word = ed_point;
					//printf("lbox_cnt=%3d , j=%3d , min_x=%3d\n", lbox_cnt,j,min_x);
				}
			}
		}

		// 右方向に単語が見つからなくなれば次の行へ移動する
		if (min_x == 999){
			printf("go to next row\n");
			lbox_cnt++;
		}
		else {
			// 角度が30度から-30度の範囲にある、かつ最も近い(X座標が)単語を次の単語とする。そこから次の単語を探すため、同じ処理を行う。
			st_point = next_word;
			printf("Add : st_point=(%3d,%3d)\n", st_point->x, st_point->y);
			// st_pointを探索済みの配列に代入する
			boxaAddBox(searched_boxes, st_point, L_CLONE);
		}
	}
}