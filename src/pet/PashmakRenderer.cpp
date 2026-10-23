#include "PetAppearance.h"
#include "../display/Display.h"

void drawPashmakBody(Display &d, PetBodyPose pose, int8_t legSwing, int8_t tailSwing, uint16_t fur, uint16_t furDark, uint16_t furLight, uint16_t cream)
{
    const bool lying = pose == PetBodyPose::LieDown;
    const bool sitting = pose == PetBodyPose::Sit;
    const int16_t bodyY = lying ? 166 : (sitting ? 163 : 158);
    d.drawLine(194, 151, 207, 145, furDark);
    d.drawLine(207 + tailSwing, 145, 215 + tailSwing, 151, furDark);
    d.fillCircle(215 + tailSwing, 153, 5, fur);
    d.fillEllipse(160, bodyY, lying ? 38 : 25, lying ? 15 : (sitting ? 20 : 25), furDark);
    d.fillEllipse(160, bodyY - 2, lying ? 35 : 22, lying ? 13 : (sitting ? 18 : 23), fur);
    if (lying) {
        d.fillEllipse(139, 174, 14, 6, furDark);
        d.fillEllipse(139, 172, 12, 4, furLight);
        d.fillEllipse(181, 174, 14, 6, furDark);
        d.fillEllipse(181, 172, 12, 4, furLight);
        d.fillEllipse(160 + legSwing, 179, 12, 5, furDark);
        d.fillEllipse(160 + legSwing, 177, 10, 4, furLight);
    } else if (sitting) {
        d.fillEllipse(143, 169, 12, 12, furDark);
        d.fillEllipse(143, 168, 9, 10, fur);
        d.fillEllipse(177, 169, 12, 12, furDark);
        d.fillEllipse(177, 168, 9, 10, fur);
        d.fillEllipse(149 + legSwing, 179, 12, 6, furDark);
        d.fillEllipse(149 + legSwing, 177, 10, 4, furLight);
        d.fillEllipse(171 - legSwing, 179, 12, 6, furDark);
        d.fillEllipse(171 - legSwing, 177, 10, 4, furLight);
    } else {
        const int16_t swing = pose == PetBodyPose::Walk || pose == PetBodyPose::Hop ? legSwing : 0;
        d.fillEllipse(137, 158, 9, 17, furDark);
        d.fillEllipse(137, 157, 6, 14, fur);
        d.fillEllipse(183, 158, 9, 17, furDark);
        d.fillEllipse(183, 157, 6, 14, fur);
        d.fillEllipse(148 + swing, 178, 10, 7, furDark);
        d.fillEllipse(148 + swing, 176, 8, 6, furLight);
        d.fillEllipse(172 - swing, 178, 10, 7, furDark);
        d.fillEllipse(172 - swing, 176, 8, 6, furLight);
    }
}
