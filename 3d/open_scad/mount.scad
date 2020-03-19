use <body.scad>

module mounting_square(x, y, z) {
    $fn = 30;
    translate([x, y, z])
    difference() {
        cube(size=[5, 20, 6]);

        translate([3, 17, 3])
        rotate([0,90,0])
        cylinder(r=1.75, h=6, center=true);

        translate([3, 7, 3])
        rotate([0,90,0])
        cylinder(r=1.75, h=6, center=true);
    }
}

module mount_leg(x, y, z, deg) {
    translate([x, y, z])
    rotate([deg, 0, 0])
    translate([3, -3, -3])
    rotate([0, 0, 90]) {
        $fn = 35;
        difference() {
            union() {
                cube(size=[33, 6, 3]);
                cube(size=[3, 6, 15]);
                translate([-15, 0, -11])
                rotate([0, 30, 0]) 
                difference() {
                    cube(size=[6, 6, 30]);

                    translate([3, 3, 3])
                    rotate([90,0,0])
                    cylinder(r=1.75, h=6, center=true);

                    translate([3, 3, 13])
                    rotate([90,0,0])
                    cylinder(r=1.75, h=6, center=true);

                }
            }
            m3_hole(10.5, 3, 0, 3);
            m3_hole(26.5, 3, 0, 3);
            translate([3,-1,7.5])
            cube(size=[3, 8, 15]);
        }
        
    }
}

module mount_plate(x, y, z, deg) {
    translate([x, y, z])
    rotate([deg, 0, 0]) {
        cube(size=[190, 20, 2]);
        mounting_square(18, 0, 2);
        mounting_square(172, 0, 2);
    }
}

mount_plate(0, 0, 0, 0);