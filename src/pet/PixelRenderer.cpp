#include "PetAppearance.h"
#include "../display/Display.h"

void drawPixelBody(Display &d, PetBodyPose pose, int8_t legSwing, int8_t tailSwing, uint16_t fur, uint16_t furDark, uint16_t furLight, uint16_t cream)
{
    const bool lying = pose == PetBodyPose::LieDown;
    const bool sitting = pose == PetBodyPose::Sit;
    d.drawLine(188, 164, 201, 151, furDark);
    d.drawLine(201 + tailSwing, 151, 207 + tailSwing, 139, furDark);
    d.drawLine(188, 163, 200 + tailSwing, 151, fur);
    d.drawLine(200 + tailSwing, 151, 206 + tailSwing, 140, fur);
    if (lying) {
        d.fillEllipse(160, 166, 31, 13, furDark);
        d.fillEllipse(160, 164, 28, 11, fur);
        d.fillEllipse(135, 174, 13, 5, furDark);
        d.fillEllipse(135, 172, 10, 3, furLight);
        d.fillEllipse(185, 174, 13, 5, furDark);
        d.fillEllipse(185, 172, 10, 3, furLight);
    } else if (sitting) {
        d.fillEllipse(160, 162, 21, 21, furDark);
        d.fillEllipse(160, 160, 18, 19, fur);
        d.fillEllipse(145, 171, 11, 10, furDark);
        d.fillEllipse(145, 169, 8, 8, fur);
        d.fillEllipse(175, 171, 11, 10, furDark);
        d.fillEllipse(175, 169, 8, 8, fur);
    } else {
        d.fillEllipse(160, 158, 20, 27, furDark);
        d.fillEllipse(160, 156, 17, 25, fur);
        d.fillEllipse(143 + (pose == PetBodyPose::Walk ? -legSwing : 0), 159 + (pose == PetBodyPose::Walk ? 2 : 0), 7, 18, furDark);
        d.fillEllipse(143, 158, 4, 16, fur);
        d.fillEllipse(177 + (pose == PetBodyPose::Walk ? legSwing : 0), 159 + (pose == PetBodyPose::Walk ? -2 : 0), 7, 18, furDark);
        d.fillEllipse(177, 158, 4, 16, fur);
    }
    const int16_t swing = pose == PetBodyPose::Walk || pose == PetBodyPose::Hop ? legSwing : 0;
    d.fillEllipse(149 + swing, 179, 8, 6, furDark);
    d.fillEllipse(149 + swing, 177, 6, 5, furLight);
    d.fillEllipse(171 - swing, 179, 8, 6, furDark);
    d.fillEllipse(171 - swing, 177, 6, 5, furLight);
    d.drawLine(153, 151, 155, 165, cream);
    d.drawLine(167, 151, 165, 165, cream);
}
