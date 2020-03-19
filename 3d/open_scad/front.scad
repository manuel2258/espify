module switch_sockel(x, y, z) {
    translate([x, y, z]) {
        translate([0, 0, 3.25]) 
        cube(size=[17, 17, 4], center=true);
            
        translate([0, 0, 0.75]) 
        cube(size=[14.65, 14.65, 1.5], center=true);
    }
}

module m3_screw_hole(x, y, z, flipped) {
    $fn = 35;
     translate([0, 0, flipped?3:0])
    translate([x, y, z + 0.5])
    rotate([flipped?180:0, 0, 0]) {
        cylinder(r=1.75, h=1, center=true);
        translate([0, 0, 2])
        cylinder(r=3, h=3, center=true);
    }
}

module key_front(x, y, z) {
    translate([x, y, z])
    union() {
        difference() {
            cube(size=[195, 30, 4]);
            for(i = [0:5]) {
                switch_sockel(20 + 26*i, 15, 0);
            }
            m3_screw_hole(4.5, 4.5, 0, false);
            m3_screw_hole(190.5, 4.5, 0, false);
            m3_screw_hole(4.5, 25.5, 0, false);
            m3_screw_hole(190.5, 25.5, 0, false);

            $fn = 35;
            translate([175, 15, 2])
            cylinder(r=3.25, h=4, center=true);
        }
        translate([3.25, 30, 0])
        cube(size=[63.5, 1, 10]);
    }
}

module disp_front(x, y, z) {
    translate([x, y, z])
        difference() {
            cube(size=[70, 50, 4]);

            translate([12, 7])
            cube(size=[46, 35, 4]);

            m3_screw_hole(4.5, 4.5, 0, false);
            m3_screw_hole(65.5, 4.5, 0, false);
            m3_screw_hole(4.5, 45.5, 0, false);
            m3_screw_hole(65.5, 45.5, 0, false);

            m3_screw_hole(9, 10, 0, false);
            m3_screw_hole(9, 38.5, 0, false);
            m3_screw_hole(61, 10, 0, false);
            m3_screw_hole(61, 38.5, 0, false);
        }
}

key_front(0, 0, 0);
disp_front(0, 30, 10);