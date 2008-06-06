#define SPECIES_NAME_LEN 10    // only for displaying speciesnames

typedef enum  {
    PH_G_STANDARD,
    PH_G_NAMES,
    PH_G_RULER,
    PH_G_RULER_DISPLAY,
    PH_G_BELOW_DIST,
    PH_G_ABOVE_DIST,
    PH_G_LAST                   // must be last
} PH_gc;

class PH_dmatrix {
    AW_pos screen_width;          // dimensions of main screen
    AW_pos screen_height;
    long   cell_width;            // width and height of one cell
    long   cell_height;
    long   cell_offset;           // don't write on base line of cell
    long   horiz_page_start;      // number of first element in this page
    long   vert_page_start;
    long   vert_last_view_start;  // last value of scrollbars
    long   horiz_last_view_start;
    long   total_cells_horiz;
    long   total_cells_vert;
    long   horiz_page_size;       // page size in cells (how many cells per page,
    long   vert_page_size;        // vertical and horizontal)
    long   off_dx;                // offset values for devic.shift_dx(y)
    long   off_dy;
    double min_view_dist;         // m[i][j]<min_view_dist -> ascii ; else small slider
    double max_view_dist;         // m[i][j]>max_view_dist -> ascii ; else small slider

    void set_scrollbar_steps(long width,long hight,long xinc,long yinc);

public:
    AW_window *awm;
    AW_device *device;          // device to draw in
    PHMATRIX  *ph_matrix;       // the Matrix

    PHMATRIX *get_matrix();

    void monitor_vertical_scroll_cb(AW_window *);   // vertical and horizontal
    void monitor_horizontal_scroll_cb(AW_window *); // scrollbar movements
    void display(void);                             // display data
    void resized(void);                             // call after resize main window

    // ******************** real public section *******************
    void set_slider_min(double d) { min_view_dist = d; };
    void set_slider_max(double d) { max_view_dist = d; };

    PH_dmatrix(void);              // constructor
    void init(PHMATRIX *matrix=0); // set the output matrix
    
    // if matrix == 0, use PHMATRIX::root
};

AW_window *PH_create_view_matrix_window(AW_root *awr, PH_dmatrix *dmatrix);
