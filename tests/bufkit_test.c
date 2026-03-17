/****************************************************************************************************************************
 *                                                   Test Loading Data
 ***************************************************************************************************************************/
void
bufkit_load_data_namm_test(void)
{
    MagAllocator alloc_ = mag_allocator_dyn_arena_create(ECO_MiB(2));
    MagAllocator *alloc = &alloc_;
    char *fname = "tests/example_data/2017040118Z_namm_kmso.buf";
    ElkStr fn_str = elk_str_from_cstring(fname);

    ElkStr namm = eco_file_slurp_text(fname, alloc);

    /* SondeSoundingList *sndgs = */ sonde_sounding_from_bufkit_str(alloc, namm, fn_str);

    eco_arena_destroy(alloc);
}

void
bufkit_load_data_gfs_test(void)
{
    MagAllocator alloc_ = mag_allocator_dyn_arena_create(ECO_MiB(2));
    MagAllocator *alloc = &alloc_;
    char *fname = "tests/example_data/2017040118Z_gfs_kmso.buf";
    ElkStr fn_str = elk_str_from_cstring(fname);

    ElkStr gfs = eco_file_slurp_text(fname, alloc);

    /* SondeSoundingList *sndgs = */ sonde_sounding_from_bufkit_str(alloc, gfs, fn_str);

    eco_arena_destroy(alloc);
}

/****************************************************************************************************************************
 *                                                   All Bufkit Tests
 ***************************************************************************************************************************/
void
all_bufkit_tests(void)
{
    printf("\t\t\tBufkit file tests...");

    bufkit_load_data_namm_test();
    bufkit_load_data_gfs_test();

    printf("completed.\n");
}

