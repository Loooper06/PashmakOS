#include "PetAppearance.h"
#include "../display/Display.h"

void drawMochiBody(Display &d, PetBodyPose pose, int8_t legSwing, int8_t tailSwing, uint16_t fur, uint16_t furDark, uint16_t furLight, uint16_t cream)
{
    const bool lying = pose == PetBodyPose::LieDown;
    const bool sitting = pose == PetBodyPose::Sit;
    d.fillEllipse(198 + tailSwing, lying ? 169 : 158, 12, 9, furDark);
    d.fillEllipse(200 + tailSwing, lying ? 167 : 156, 9, 7, furLight);
    const int16_t bodyY = lying ? 166 : (sitting ? 163 : 159);
    d.fillEllipse(160, bodyY, lying ? 41 : 29, lying ? 15 : (sitting ? 20 : 25), furDark);
    d.fillEllipse(160, bodyY - 2, lying ? 38 : 26, lying ? 13 : (sitting ? 18 : 23), fur);
    if (lying) {
        d.fillEllipse(136, 174, 15, 6, furDark);
        d.fillEllipse(136, 172, 12, 4, cream);
        d.fillEllipse(184, 174, 15, 6, furDark);
        d.fillEllipse(184, 172, 12, 4, cream);
        d.fillEllipse(160 + legSwing, 179, 13, 5, furDark);
        d.fillEllipse(160 + legSwing, 177, 10, 4, cream);
    } else if (sitting) {
        d.fillEllipse(135, 168, 12, 12, furDark);
        d.fillEllipse(135, 166, 9, 10, fur);
        d.fillEllipse(185, 168, 12, 12, furDark);
        d.fillEllipse(185, 166, 9, 10, fur);
        d.fillEllipse(147 + legSwing, 179, 12, 6, furDark);
        d.fillEllipse(147 + legSwing, 177, 9, 4, cream);
        d.fillEllipse(173 - legSwing, 179, 12, 6, furDark);
        d.fillEllipse(173 - legSwing, 177, 9, 4, cream);
    } else {
        const int16_t swing = pose == PetBodyPose::Walk || pose == PetBodyPose::Hop ? legSwing : 0;
        d.fillEllipse(134 + (pose == PetBodyPose::Walk ? -legSwing : 0), 158 + (pose == PetBodyPose::Walk ? 2 : 0), 10, 15, furDark);
        d.fillEllipse(134, 157, 7, 13, fur);
        d.fillEllipse(186 + (pose == PetBodyPose::Walk ? legSwing : 0), 158 + (pose == PetBodyPose::Walk ? -2 : 0), 10, 15, furDark);
        d.fillEllipse(186, 157, 7, 13, fur);
        d.fillEllipse(146 + swing, 178 + (pose == PetBodyPose::Walk ? swing / 2 : 0), 11, 7, furDark);
        d.fillEllipse(146 + swing, 176, 9, 6, cream);
        d.fillEllipse(174 - swing, 178 - (pose == PetBodyPose::Walk ? swing / 2 : 0), 11, 7, furDark);
        d.fillEllipse(174 - swing, 176, 9, 6, cream);
    }
    d.fillEllipse(160, lying ? 166 : 162, 13, lying ? 8 : 16, cream);
}
