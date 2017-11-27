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


// 2��BOX���r����֐�
bool  Date_equal(BOX* box1, BOX* box2){
	//if (box1 == NULL || box2 == NULL) return false;
	return box1->x == box2->x && box1->y == box2->y && box1->w == box2->w && box1->h == box2->h;
}

// �ł�������(����Ɉʒu����)�P������߂�֐�
BOX* getMinBox(BOX* min_box, BOX* box){
	if (box == NULL) return min_box;
	if (box->x <= min_box->x && box->y <= min_box->y){
		min_box = box;
	}
	return min_box;
}

BOX* getMinBox(BOX* min_box, Boxa* boxes){
	for (int i = 0; i < boxes->n; i++) {
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		if (box->x < min_box->x && box->y < min_box->y){
			min_box = box;
		}
	}
	return min_box;
}

// ���̍s�̐擪��T���֐�
BOX* getNextRow(BOX* min_box, Boxa* boxes){
	int nearest_y = 999;
	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		// ���݂̊�l��荶���Ɉʒu����
		if (box->x < min_box->x){
			// ���݂̊�l��艺���Ɉʒu����
			if (box->y > min_box->y){
				// ���̒��ł����݂̊�l����ł�Y�����ɋ߂��Ɉʒu����
				if (box->y < nearest_y){
					nearest_y = box->y;
					min_box = box;
				}
			}
		}
	}
	return min_box;
}

// box��boxes�̒��ɂ����true�A�����łȂ����false��Ԃ�
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

// a_boxes�̗v�f��������ׁAb_boxes�̗v�f�łȂ��Ȃ�Ac_boxes�ɉ�����
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
	int rparam_end = 893;
	int break_flg = 0;
	int nearest_y = 999;

	Boxa* valid_boxes = boxaCreate(50);
	Boxa* extracted_boxes = boxaCreate(50);
	Boxa* unvisit_boxes = boxaCreate(50);

	BOX* box0 = boxCreate(119, 65, 9, 6);		// 1�s��
	BOX* box1 = boxCreate(363, 38, 37, 19);		// 
	BOX* box2 = boxCreate(410, 29, 110, 25);	// 
	BOX* box3 = boxCreate(529, 28, 74, 21);		// 
	BOX* box4 = boxCreate(102, 144, 65, 30);	// 2�s��
	BOX* box5 = boxCreate(176, 142, 32, 23);	//

	BOX* min_box = boxCreate(999, 999, 0, 0);

	boxaAddBox(valid_boxes, box0, L_CLONE);
	boxaAddBox(valid_boxes, box1, L_CLONE);
	boxaAddBox(valid_boxes, box2, L_CLONE);
	boxaAddBox(valid_boxes, box3, L_CLONE);
	boxaAddBox(valid_boxes, box4, L_CLONE);
	boxaAddBox(valid_boxes, box5, L_CLONE);

	// �ŏ��l�����߂�(1�s�ڂ̕��͂̍ł�����)
	min_box = getMinBox(min_box,valid_boxes);
	boxaAddBox(extracted_boxes, min_box, L_CLONE);
	printf("init min_box    : i=  0 , box=(%3d,%3d) , valid_boxes->n=%3d\n", min_box->x, min_box->y, valid_boxes->n);

	// �P������ׂđ�������܂Ń��[�v
	for (int i = 1; i < valid_boxes->n; i++){
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		//printf("box=(%3d,%3d,%3d,%3d)\n", box->x, box->y, box->w, box->h);
		// �����{�b�N�X����̏ꍇ�̏�����ǉ� **
		// ��_����E�����ɒP���T��
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					printf("word extracted  : i=%3d , box=(%3d,%3d)\n", i, j, k);
					// ���̍s�Ɋ܂܂��P��Ƃ��Ĕz��Ɋi�[
					boxaAddBox(extracted_boxes, box, L_CLONE);
					break_flg = 1;
					break;
				}
			} // ���[�v��E����
			if (break_flg == 1) break;
		}

		// ��l���E�����ɒP�ꂪ���݂��Ȃ������ꍇ
		if (break_flg == 0){			
			// ����܂łɌ������P���������������A�ŏ�(�ł�����Ɉʒu����)�̒P���������B��������̊�l�Ƃ���
			set_difference(valid_boxes, extracted_boxes, unvisit_boxes);
			min_box = getNextRow(min_box,unvisit_boxes);
			printf("not  extracted  : i=%3d\n", i);
			printf("valid_boxes->n=%3d , extracted_boxes->n=%3d, unvisit_boxes->n=%3d\n", valid_boxes->n, extracted_boxes->n, unvisit_boxes->n);
			printf("next rows point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			boxaAddBox(extracted_boxes, min_box, L_CLONE);
		}

		// �P�ꂪ�������ă��[�v��E�����ꍇ
		if (break_flg == 1){
			// �t���O��������
			break_flg = 0;
			// �������P��̍��W�����̊�l�Ɏw��
			min_box = box;
			printf("next word point : min_box=(%3d,%3d,%3d,%3d)\n", min_box->x, min_box->y, min_box->w, min_box->h);
		}
	}

	printf("\nunvisit_boxes->n=%3d\n", unvisit_boxes->n);
	for (int i = 0; i < unvisit_boxes->n; i++) {
		BOX* box = boxaGetBox(unvisit_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}

	printf("extracted_boxes->n=%3d\n", extracted_boxes->n);
	for (int i = 0; i < extracted_boxes->n; i++) {
		BOX* box = boxaGetBox(extracted_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}

}