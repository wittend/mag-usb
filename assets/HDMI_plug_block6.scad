// Pi Eliminator Box 
// 2025-06-13 DMW
//==================================
include <MCAD/boxes.scad>
// include <BOLTS.scad>
//include </home/dave/.local/share/OpenSCAD/libraries/BOLTS.scad>
// Wall Thickness
wallTh = 1.0;

// Inner dimensions
iLen    = 25.0 - wallTh;
iWid    = 30.0 - wallTh;
iHt     = 30.0;

// Outer Dimensions 
oLen    = 32.0;
oWid    = 32.0;
//oHt  = 34.0 + (2 * wallTh);
oHt     = 13.0;

// Plug
pLen    = 20;
pWid    = 21.1;
pHt     = 10.1;

$fn = 50;

module boltholes()
{
    //ISO4014("M2", 5);
    //echo(get_dim(DIN125A_dims("M4"),"d1"));
    //echo(get_dim(dims,"d1"));
}

module main()
{
    // Main box
    difference()
    {
        // Outer dimensions
        cube([oLen, oWid, oHt], center = false);
        translate([2*wallTh, -(2*wallTh), wallTh+1])
        {
            // Inner region
            cube([oLen, oWid, oHt], center = false);
        }
//        translate([(pWid/2) - (6*wallTh), pLen - (2*wallTh), (pHt/2) + (2*wallTh)])
//        translate([(pWid/2) - (5*wallTh), pLen - (2*wallTh), (pHt/2) + (wallTh/2)])
        translate([((pWid/2) + 0.5) - (5*wallTh), pLen - (2*wallTh), (pHt/2)-2.5])
        {
            // Cutout for plug
            cube([pWid - 1.0, pLen, pHt+2], center = false);
        }
        translate([-1, -10, (2*wallTh)])
        {
            // Cutout for right side
            cube([4, pLen * 2, 15], center = false);
        }
        // Screw Holes
        translate([2.9, oLen, oHt/2 + 1 ])
        {
            rotate([90, 0, 0])
            {
                cylinder(h=10, d=3.35, center=true);
            }
        }
        translate([29.0, oLen, oHt/2 + 1 ])
        {
            rotate([90, 0, 0])
            {
                cylinder(h=10, d=3.35, center=true);
            }
        }
    }
}

main();