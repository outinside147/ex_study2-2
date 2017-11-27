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
	int nearest_y = 9999;
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
	// ���ʂ��e�L�X�g�ɏo��
	ofstream content("../image/component.txt");
	ofstream ex_boxes("../image/extracted_boxes.txt");
	ofstream uv_boxes("../image/unvisit_boxes.txt");
	ofstream result("../image/result.txt");

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
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //�i��
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL); //�s
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL); //�P��
	printf("Found textline=%3d , word=%3d\n", line_boxes->n, word_boxes->n);

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // �i���̍��[
		rparam_end = box->x + box->w; //�i���̉E�[
		api->SetRectangle(box->x, box->y, box->w, box->h);
	}

	// �s�P�ʂł̔F�����ʂ������o��
	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
	}


	Box* min_box = boxCreate(src.cols, src.rows, 0, 0); // �ŏ��l(�ł�����Ɉʒu����)���i�[����ϐ�
	Boxa* extracted_boxes = boxaCreate(50); // 1�s�̒P��� (���͂̍��ォ�璲�����Ď擾�������ŒP����i�[����z��)
	//Boxa* unvisit_boxes = boxaCreate(50); // �������̒P�����i�[����z��
	Boxa* unvisit_boxes = nullptr;

	// ���炩�Ɍ�F�����Ă���P�����������
	Boxa* valid_boxes = boxaCreate(line_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 3  && box->w > 3){
			// �ł�����(�ŏ�)�̍��W�����P��̈�(��l)�𒊏o����
			min_box = getMinBox(min_box, box);
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}
	// �ŏ��̒P���1�s�ڂ̊�l�Ƃ��ĕۑ�����
	boxaAddBox(extracted_boxes,min_box,L_CLONE);
	printf("init min_box : i=0 , box=(%3d,%3d) , valid_boxes->n=%3d\n", min_box->x, min_box->y, valid_boxes->n);
	printf("lparam_end=%4d, rparam_end=%4d\n", lparam_end, rparam_end);

	// �P��P�ʂł̔F�����ʂ������o��
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
	}

	int i = 0;
	int break_flg = 0;
	int nearest_y = 999;

	// �e�s���i�[����z�� Boxa�^��extracted_boxes���i�[
	// �ҏW�������� **
	Boxaa* extracted_lines = boxaaCreate(100);
	// �P������ׂđ�������܂Ń��[�v
	for (int i = 1; i < valid_boxes->n; i++){
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		// �����{�b�N�X����̏ꍇ�̏�����ǉ� **
		// ��_����E�����ɒP���T��
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					printf("word extracted : i=%3d , box=(%3d,%3d)\n", i, j, k);
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
			unvisit_boxes = boxaCreate(50);
			set_difference(valid_boxes, extracted_boxes, unvisit_boxes);
			printf("unvisit_boxes->n=%3d\n",unvisit_boxes->n);
			min_box = getNextRow(min_box, unvisit_boxes);
			printf("not extracted : i=%3d\n", i);
			printf("next row point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			boxaAddBox(extracted_boxes, min_box, L_CLONE);
			result << "break_flg=" << break_flg << ", i=" << i << ", box= (" << box->x << "," << box->y << ")" << ", next point= (" << min_box->x << "," << min_box->y << ")" << endl;
		}

		// �P�ꂪ�������ă��[�v��E�����ꍇ
		if (break_flg == 1){
			// �������P��̍��W�����̊�l�Ɏw��
			min_box = box;
			printf("next word point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			result << "break_flg=" << break_flg << ", i=" << i << ", box= (" << box->x << "," << box->y << ")" << ", next point= (" << min_box->x << "," << min_box->y << ")" << endl;
			// �t���O��������
			break_flg = 0;
		}
	}

	printf("extracted_boxes->n=%3d\n", extracted_boxes->n);
	ex_boxes << "extracted_boxes->n=" << extracted_boxes->n << endl;
	for (int i = 0; i < extracted_boxes->n; i++) {
		BOX* box = boxaGetBox(extracted_boxes, i, L_CLONE);
		ex_boxes << "i=" << i << ", box= (" << box->x << "," << box->y << "," << box->w << "," << box->h << ")" << endl;
	}

	printf("unvisit_boxes->n=%3d\n", unvisit_boxes->n);
	uv_boxes << "unvisit_boxes->n=" << unvisit_boxes->n << endl;
	for (int i = 0; i < unvisit_boxes->n; i++) {
		BOX* box = boxaGetBox(unvisit_boxes, i, L_CLONE);
		uv_boxes << "i=" << i << ", box= (" << box->x << "," << box->y << "," << box->w << "," << box->h << ")" << endl;
	}
}