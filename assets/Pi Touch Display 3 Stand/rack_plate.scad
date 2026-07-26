/**
 * OpenSCAD Rack Plate Library
 * 
 * Conforms to standards defined in Wikipedia for Rack Units (19-inch racks).
 * Formula for height: h = 44.45 * n - 0.79 (mm)
 * Standard mounting width: 465.14 mm (center-to-center)
 */

// --- Default Values ---
DEFAULT_U_COUNT = 1;
DEFAULT_THICKNESS = 3;
DEFAULT_WIDTH_INCHES = 6.5;
DEFAULT_CORNER_RADIUS = 2.5;
DEFAULT_HOLE_DIAMETER = 6;
DEFAULT_SLOT_LENGTH = 12; // At least twice the diameter (6mm * 2)

// --- Modules ---

/**
 * Generates a rack plate.
 * 
 * @param u_count Number of rack units (n)
 * @param thickness Plate thickness in mm
 * @param width_inches Nominal width of the rack in inches (default 19)
 * @param corner_radius Radius for the outer corners in mm
 * @param hole_diameter Diameter of the mounting holes in mm
 * @param slot_length Total length of the oblong mounting holes in mm
 */
module rack_plate(
    u_count         = DEFAULT_U_COUNT,
    thickness       = DEFAULT_THICKNESS,
    width_inches    = DEFAULT_WIDTH_INCHES,
    corner_radius   = DEFAULT_CORNER_RADIUS,
    hole_diameter   = DEFAULT_HOLE_DIAMETER,
    slot_length     = DEFAULT_SLOT_LENGTH
) {
    // Requirements validation
    assert(width_inches >= 5.5 && width_inches <= 23, "Width must be between 5.5 and 23 inches");

    // Constants based on requirements
    u_height_mm = 44.45;
    height_offset_mm = 0.79;
    
    // Formula: h = 44.45 * n - 0.79
    total_height = (u_height_mm * u_count) - height_offset_mm;
    
    // 19-inch rack mounting width (center to center)
    // Wikipedia: 18.313 in = 465.14 mm
    standard_mounting_width = 465.14; 
    standard_total_width = 19 * 25.4; // 482.6 mm
    
    // Total width of the plate. 
    total_width = width_inches * 25.4;
    
    // Flange width calculation (distance from edge to mounting hole center)
    // "It is critical that the mounting holes retain their position relative to the adjacent ends of the panel"
    // For a standard 19" rack, the offset from edge to hole center is:
    edge_to_hole_offset = (standard_total_width - standard_mounting_width) / 2;

    linear_extrude(height = thickness) 
    {
        difference() 
        {
            // Main Plate Body with rounded corners
            rounded_rect(total_width, total_height, corner_radius);

            // Mounting Holes
            // "The code should generate flanges with 2 rather than three holes per 1 U of height."
            if (u_count >= 1.0) 
            {
                for (u = [0 : floor(u_count) - 1]) 
                {
                    u_base_y = u * u_height_mm;
                    hole_spacing = 31.75; 
                    y_offset = (u_height_mm - hole_spacing) / 2;
                    
                    y_pos1 = u_base_y + y_offset;
                    y_pos2 = u_base_y + y_offset + hole_spacing;
    
                    // Center holes in the total_height
                    // Since total_height = u_count * 44.45 - 0.79, 
                    // the gaps are at the top and bottom of the stack.
                    // We shift everything by (0.79 / 2) to center it if we consider 
                    // the 0.79 to be distributed.
                    // Actually, the previous code had a formula for center_shift.
                    total_nominal_height = u_height_mm * u_count;
                    center_shift = (total_nominal_height - total_height) / 2;
                    
                    p1 = y_pos1 - center_shift;
                    p2 = y_pos2 - center_shift;
    
                    // Left Holes (Oblong)
                    translate([edge_to_hole_offset, p1])
                    {
                        oblong_hole(hole_diameter, slot_length);
                    }
                    translate([edge_to_hole_offset, p2]) 
                    {
                        oblong_hole(hole_diameter, slot_length);
                    }   
                    // Right Holes (Oblong)
                    translate([total_width - edge_to_hole_offset, p1]) 
                    {
                        oblong_hole(hole_diameter, slot_length);
                    }
                    translate([total_width - edge_to_hole_offset, p2]) 
                    {
                        oblong_hole(hole_diameter, slot_length);
                    }
                }
            }
            else if(u_count < 1.0)
            {
                // There can only be one mounting hole on each side, if any.
                u = 0;
                u_base_y = u * u_height_mm/2;
                hole_spacing = 31.75; 
                y_offset = (u_height_mm - hole_spacing) / 2;
                
                y_pos1 = u_base_y + u_height_mm/4;

                // Center holes in the total_height
                // Since total_height = u_count * 44.45 - 0.79, 
                // the gaps are at the top and bottom of the stack.
                // We shift everything by (0.79 / 2) to center it if we consider 
                // the 0.79 to be distributed.
                // Actually, the previous code had a formula for center_shift.
                total_nominal_height = u_height_mm * u_count;
                center_shift = (total_nominal_height - total_height) / 2;
                
                p1 = y_pos1 - center_shift;

                // Left Holes (Oblong vertical)
                translate([edge_to_hole_offset,  p1])
                {
                    rotate([0, 0, 90])
                    {
                        oblong_hole(hole_diameter, slot_length);
                    }
                }
                    
                // Right Holes (Oblong vertical)
                translate([total_width - edge_to_hole_offset, p1])
                {
                    rotate([0, 0, 90])
                    {
                        oblong_hole(hole_diameter, slot_length);
                    }
                }
            }
        }
    }
}

/**
 * Helper for oblong (slotted) hole
 */
module oblong_hole(d, l) 
{
    hull() 
    {
        translate([-(l - d) / 2, 0]) circle(d = d, $fn = 24);
        translate([(l - d) / 2, 0]) circle(d = d, $fn = 24);
    }
}

/**
 * Helper for rounded rectangle 2D shape
 */
module rounded_rect(w, h, r) 
{
    translate([r, r, 0])
    {
        minkowski() 
        {
            square([w - 2 * r, h - 2 * r]);
            circle(r = r, $fn = 48);
        }
    } 
}

// --- Example Usage ---
// This will render when the file is opened directly in OpenSCAD
// but won't interfere when included in other files.

rack_plate(u_count = 0.75, width_inches= 6.5);

