use <front.scad>;
use <body.scad>;
use <mount.scad>;

rotate([30, 0, 0]) {
    body();
    key_front(0, 0, 20);
    disp_front(0, 30, 30);
}

mount_leg(15, 0, 0, 30);
mount_leg(180, 0, 0, 30);
