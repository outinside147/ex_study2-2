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


// 2つのBOXを比較する関数
bool  Date_equal(BOX* box1, BOX* box2){
	//if (box1 == NULL || box2 == NULL) return false;
	return box1->x == box2->x && box1->y == box2->y && box1->w == box2->w && box1->h == box2->h;
}

int main()
{
	int rparam_end = 893;
	int break_flg = 0;
	int nearest_y = 999;

	Boxa* suit_Wboxes = boxaCreate(6);
	BOX* box0 = boxCreate(119, 65, 9, 6);		// 1行目
	BOX* box1 = boxCreate(363, 38, 37, 19);		// 
	BOX* box2 = boxCreate(410, 29, 110, 25);	// 
	BOX* box3 = boxCreate(529, 28, 74, 21);		// 
	BOX* box4 = boxCreate(102, 144, 65, 30);	// 2行目
	BOX* box5 = boxCreate(176, 142, 32, 23);	//

	BOX* min_box = boxCreate(999, 999, 0, 0);

	Boxa* correct_boxes = boxaCreate(50);

	boxaAddBox(suit_Wboxes, box0, L_CLONE);
	boxaAddBox(suit_Wboxes, box1, L_CLONE);
	boxaAddBox(suit_Wboxes, box2, L_CLONE);
	boxaAddBox(suit_Wboxes, box3, L_CLONE);
	boxaAddBox(suit_Wboxes, box4, L_CLONE);
	boxaAddBox(suit_Wboxes, box5, L_CLONE);

	for (int i = 0; i < suit_Wboxes->n; i++) {
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		if (box->x <= min_box->x && box->y <= min_box->y){
			min_box = box;
		}
	}
	boxaAddBox(correct_boxes, min_box, L_CLONE);
	printf("init : min_box=(%3d,%3d,%3d,%3d) , suit_Wboxes->n=%3d \n", min_box->x, min_box->y, min_box->w, min_box->h, suit_Wboxes->n);

	// 単語をすべて走査するまでループ
	for (int i = 1; i < suit_Wboxes->n; i++){
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		//printf("box=(%3d,%3d,%3d,%3d)\n", box->x, box->y, box->w, box->h);
		// 得たボックスが空の場合の処理を追加 **
		// 基準点から右方向に単語を探索
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					printf("word extracted : i=%3d , box=(%3d,%3d)\n", i, j, k);
					// この行に含まれる単語として配列に格納
					boxaAddBox(correct_boxes, box, L_CLONE);
					break_flg = 1;
					break;
				}
			} // ループを脱する
			if (break_flg == 1) break;
		}
		
		if (break_flg == 0){
			// 基準値か右方向に単語が存在しなかった場合
			printf("not extracted : i=%3d\n", i);
			// これまでに見つけた単語を除いた中から、最小(最も左上に位置する)の単語を見つける。それを次の基準値とする
			printf("suit_Wboxes->n=%3d , correct_boxes->n=%3d\n", suit_Wboxes->n, correct_boxes->n);
			for (int m = 0; m < suit_Wboxes->n; m++){
				BOX* org_box = boxaGetBox(suit_Wboxes, m, L_CLONE);
				for (int p = 0; p < correct_boxes->n; p++){
					BOX* c_box = boxaGetBox(correct_boxes, p, L_CLONE);
					printf("(m,p)=(%3d,%3d) , org_box=(%3d,%3d) , c_box=(%3d,%3d)\n", m, p, org_box->x, org_box->y, c_box->x, c_box->y);
					if (Date_equal(org_box, c_box)){ //trueの場合
						printf(" - break - \n");
						break;
					} else if (!Date_equal(org_box,c_box)){ //falseの場合
						// 最小値の座標を取得する
						// 現在探索している単語の座標より左側にある
						if (org_box->x < min_box->x){
							// 現在探索している単語の座標より下側にある
							if (org_box->y > min_box->y){
								// これまでに探索した単語を除いて、もっとも上にある単語を次の基準値とする
								if (org_box->y < nearest_y){
									//printf("nearest_y=%3d , org_box=(%3d,%3d)\n",nearest_y,org_box->x,org_box->y);
									nearest_y = org_box->y;
									min_box->y = nearest_y;
									min_box->x = org_box->x;
									min_box->w = org_box->w;
									min_box->h = org_box->h;
									printf("nearest_y=%3d\n", nearest_y);
								}
							}
						}
						//printf("min_box=(%3d,%3d,%3d,%3d)\n", min_box->x, min_box->y, min_box->w, min_box->h);
					}
				}
			}
			printf("next rows point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			boxaAddBox(correct_boxes, min_box, L_CLONE);
		}

		// 単語が見つかってループを脱した場合
		if (break_flg == 1){
			// フラグを初期化
			break_flg = 0;
			// 見つけた単語の座標を次の基準値に指定
			min_box = box;
			printf("next word point : min_box=(%3d,%3d,%3d,%3d)\n", min_box->x, min_box->y, min_box->w, min_box->h);
		}

		/*
		else if (break_flg == 0){ // 単語が見つからずにループを脱した場合
			// この場合、文章の右端まで走査し終えた。よって次の行の左上端の座標を見つける
			// これまでに見つけた単語を除いたものから、最小(最も左上)の座標にある単語を見つける
			min_box->x = 999;
			min_box->y = 999;
			min_box->w = 0;
			min_box->h = 0;
			for (int m = 0; m < suit_Wboxes->n; m++) {
				BOX* org_box = boxaGetBox(suit_Wboxes, m, L_CLONE);
				for (int p = 0; p < correct_boxes->n; p++){
					BOX* c_box = boxaGetBox(correct_boxes, p, L_CLONE);
					// 全配列と抽出した配列を比較し、含まれていない場合は比較対象とする
					// Date_equal関数を呼び出して、その結果がfalseの場合のみ最小値の更新を行う
					if (!Date_equal(org_box, c_box)){
						// 最小値の座標を取得する
						min_box = getMinBox(min_box, org_box);
					}
				}
			}
			printf("min_box=(%3d,%3d,%3d,%3d)\n", min_box->x, min_box->y, min_box->w, min_box->h);
			i++;
			printf("i=%3d\n", i);
		}
		*/
	}

	printf("correct_boxes->n = %3d\n", correct_boxes->n);
	for (int i = 0; i < correct_boxes->n; i++) {
		BOX* box = boxaGetBox(correct_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}
}