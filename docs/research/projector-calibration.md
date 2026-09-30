# Projector ranges: hit detection and calibration

Research note for the projector/laser calibration adapter (ROADMAP, Later). Felipe's brief: support every detection method and screen type the leading systems use, and do the shot-to-image mapping better than a friend's company did, whose fix was an equation Felipe does not recall.

## Detection methods in use

| Method | How the hit point is found | Seen in |
| --- | --- | --- |
| Laser flash + fixed camera(s) | Weapon emits a laser pulse on trigger; one or more cameras watching the screen find the spot | VirTra (sensor array over a multi-projector panoramic screen) [1]; InVeris FATS LIVE (dual-camera hit detection, also used for live fire on a self-healing rubber screen) [2] |
| Continuous IR spot tracking | Weapon places an infrared spot on the screen; a tracker reports its X, Y continuously, which also gives aim traces | US patent 5215465, "Infrared spot tracker" [3] |
| Camera on weapon | Camera on the weapon sees markers on the screen; a homography maps them to the hit point | Academic camera-on-weapon simulators [3] |
| Weapon and trainee pose tracking | Optical or ultra-wideband tracking gives weapon and trainee pose in the room | FAAC MILO Marksmanship Trainer [3] |

## How calibration is done

- **Single homography.** Low-cost and academic simulators map camera pixels to projector pixels with one planar homography. One reported system reaches about ±3 pixels on a 1024×768 slide with a low-resolution camera [4]. A homography models a pinhole camera looking at a flat screen only: it cannot represent lens distortion in the camera or projector, a curved or wraparound screen, or several blended projectors.
- **Parametric lens models.** Radial and tangential distortion terms fitted per lens. Better, but a global polynomial misses local distortion, and each device and screen shape needs its own model.
- **Dense structured light.** Projecting Gray-code and phase-shift patterns gives a per-pixel camera-to-projector correspondence. It works with significant nonlinear distortion in both camera and projector and on planar or freeform surfaces [5], scales to multi-projector and curved screens [6], and reaches sub-pixel accuracy: 0.312 pixel in one method, and a sub-pixel distortion lookup table improved precision by at least 95.7% in another [7]. Pixel-wise models capture local distortion that global models miss [7].

## Geometry beyond the lens (analysis, not from the sources)

Even a perfect camera-to-projector map only says which image pixel was hit. Ogiva's solver needs a 3D aim ray from the muzzle. That requires the screen surface in room coordinates, the muzzle position (tracked, or assumed from the firing point), and the rendered view drawn from the shooter's eye with an off-axis projection. If the sim instead casts a ray from its virtual camera through the hit pixel, the error changes with where the shooter stands and grows toward the image edges. Whether this was the problem Felipe's friend fought is unknown.

## Direction to decide

A calibration adapter built on dense structured-light correspondences (lens-model free, works for any screen shape and projector count), plus an explicit room model (screen surface, projectors, cameras, firing point) that turns a detected hit into a muzzle ray in Ogiva's ADR 0007 frame. Detection front ends (fixed camera, IR tracker, camera on weapon, pose tracking) would plug into the same room model. Scope, priority and module placement need Felipe's decisions and an ADR.

## Sources

1. VirTra 300 description, RECOIL: <https://www.recoilweb.com/a-tale-of-two-simulators-virtra-300-vs-chimeraxr-mythos-172299.html>; VirTra blog: <https://www.virtra.com/realistic-firearm-practice-with-laser-technology-blog/>
2. InVeris FATS LIVE: <https://www.inveristraining.com/virtual-training/public-safety-virtual-weapons-and-scenario-training/fats-live/>
3. US 5215465 "Infrared spot tracker": <https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/5215465>; FAAC MILO MMT: <https://www.faac.com/milo/virtual/mmt/>
4. Laser Actuated Presentation System: <https://arxiv.org/pdf/0911.5404>
5. Projector optical distortion calibration using Gray code patterns: <https://www.researchgate.net/publication/224165343_Projector_optical_distortion_calibration_using_Gray_code_patterns>
6. Automatic registration of multi-projector on curved screens via coded structured light: <https://doi.org/10.3390/sym11111397>
7. Camera-projector calibration with distortion compensation, comparative study: <https://www.researchgate.net/publication/328105377_Camera-Projector_Calibration_Methods_with_Compensation_of_Geometric_Distortions_in_Fringe_Projection_Profilometry_A_Comparative_Study>; polynomial distortion representation: <https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4634452/>
