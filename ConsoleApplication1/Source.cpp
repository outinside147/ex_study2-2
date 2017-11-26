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
	/* ��������ҏW **
	if(box->x < min_box->x){
		if(box->y > min_box->y){
			if(box->y < MIN){
				MIN = box->y;
			}
		}
	}
	*/

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

	Boxa* suit_Wboxes = boxaCreate(6);
	Boxa* correct_boxes = boxaCreate(50);
	Boxa* dif_boxes = boxaCreate(50);

	BOX* box0 = boxCreate(119, 65, 9, 6);		// 1�s��
	BOX* box1 = boxCreate(363, 38, 37, 19);		// 
	BOX* box2 = boxCreate(410, 29, 110, 25);	// 
	BOX* box3 = boxCreate(529, 28, 74, 21);		// 
	BOX* box4 = boxCreate(102, 144, 65, 30);	// 2�s��
	BOX* box5 = boxCreate(176, 142, 32, 23);	//

	BOX* min_box = boxCreate(999, 999, 0, 0);

	boxaAddBox(suit_Wboxes, box0, L_CLONE);
	boxaAddBox(suit_Wboxes, box1, L_CLONE);
	boxaAddBox(suit_Wboxes, box2, L_CLONE);
	boxaAddBox(suit_Wboxes, box3, L_CLONE);
	boxaAddBox(suit_Wboxes, box4, L_CLONE);
	boxaAddBox(suit_Wboxes, box5, L_CLONE);

	// �ŏ��l�����߂�(1�s�ڂ̕��͂̍ł�����)
	min_box = getMinBox(min_box,suit_Wboxes);
	boxaAddBox(correct_boxes, min_box, L_CLONE);
	printf("init : min_box=(%3d,%3d,%3d,%3d) , suit_Wboxes->n=%3d \n", min_box->x, min_box->y, min_box->w, min_box->h, suit_Wboxes->n);

	// �P������ׂđ�������܂Ń��[�v
	for (int i = 1; i < suit_Wboxes->n; i++){
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		//printf("box=(%3d,%3d,%3d,%3d)\n", box->x, box->y, box->w, box->h);
		// �����{�b�N�X����̏ꍇ�̏�����ǉ� **
		// ��_����E�����ɒP���T��
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					printf("word extracted : i=%3d , box=(%3d,%3d)\n", i, j, k);
					// ���̍s�Ɋ܂܂��P��Ƃ��Ĕz��Ɋi�[
					boxaAddBox(correct_boxes, box, L_CLONE);
					break_flg = 1;
					break;
				}
			} // ���[�v��E����
			if (break_flg == 1) break;
		}

		if (break_flg == 0){
			// ��l���E�����ɒP�ꂪ���݂��Ȃ������ꍇ
			printf("not extracted : i=%3d\n", i);
			printf("suit_Wboxes->n=%3d , correct_boxes->n=%3d, dif_boxes->n=%3d\n", suit_Wboxes->n, correct_boxes->n, dif_boxes->n);
			// ����܂łɌ������P���������������A�ŏ�(�ł�����Ɉʒu����)�̒P���������B��������̊�l�Ƃ���
			set_difference(suit_Wboxes, correct_boxes, dif_boxes);
			// ��������ҏW **
			printf("next rows point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			boxaAddBox(correct_boxes, min_box, L_CLONE);
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

	printf("dif_boxes->n=%3d\n", dif_boxes->n);
	for (int i = 0; i < dif_boxes->n; i++) {
		BOX* box = boxaGetBox(dif_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}

}