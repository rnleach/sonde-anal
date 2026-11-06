/****************************************************************************************************************************
 *                                                  Test Freezing Levels
 ***************************************************************************************************************************/
void
freezing_levels_test(char const *fname)
{
#define PRINT_DEBUG 0

    MagAllocator alloc_ = mag_allocator_dyn_arena_create(ECO_MiB(2));
    MagAllocator *alloc = &alloc_;
    ElkStr fn_str = elk_str_from_cstring((char *)fname);

    ElkStr namm = eco_file_slurp_text(fname, alloc);

    SondeSoundingList *sndgs = sonde_sounding_from_bufkit_str(alloc, namm, fn_str);

#if PRINT_DEBUG
        printf("%s\n", fname);
#endif

    SondeHectopascal const pmin = (SondeHectopascal){ .val = 500.0 };
    size idx = 0;
    for(SondeSoundingList *sl = sndgs; sl; sl = sl->next, ++idx)
    {
        SondeSounding *snd = sl->snd;
        SondeLevels *fz_levels = sonde_freezing_levels(snd, pmin, alloc);

        Assert(fz_levels);
        Assert(fz_levels->n_levels >= 0);

#if PRINT_DEBUG
        printf("%3ld: %ld levels  ", idx, fz_levels->n_levels);
#endif

        for(size l = 0; l < fz_levels->n_levels; ++l)
        {
            SondeLevel *lvl = &fz_levels->data[l];
            Assert(lvl->p.val > 0.0);
            Assert(lvl->t.val == 0.0);

#if PRINT_DEBUG
            printf("%.0lf hPa,  ", lvl->p.val);
#endif

        }

#if PRINT_DEBUG
        printf("\n");
#endif

    }

#if PRINT_DEBUG
        printf("\n");
#endif

    eco_arena_destroy(alloc);
#undef PRINT_DEBUG
}

void
freezing_levels_all_tests(void)
{
    freezing_levels_test("tests/example_data/2017040118Z_namm_kmso.buf");
    freezing_levels_test("tests/example_data/2017040118Z_gfs_kmso.buf");
}

/****************************************************************************************************************************
 *                                               Test Wet Bulb Zero Levels
 ***************************************************************************************************************************/
void
wbz_levels_test(char const *fname)
{
#define PRINT_DEBUG 0

    MagAllocator alloc_ = mag_allocator_dyn_arena_create(ECO_MiB(2));
    MagAllocator *alloc = &alloc_;
    ElkStr fn_str = elk_str_from_cstring((char *)fname);

    ElkStr namm = eco_file_slurp_text(fname, alloc);

    SondeSoundingList *sndgs = sonde_sounding_from_bufkit_str(alloc, namm, fn_str);

#if PRINT_DEBUG
        printf("%s\n", fname);
#endif

    SondeHectopascal const pmin = (SondeHectopascal){ .val = 500.0 };
    size idx = 0;
    for(SondeSoundingList *sl = sndgs; sl; sl = sl->next, ++idx)
    {
        SondeSounding *snd = sl->snd;
        SondeLevels *fz_levels = sonde_wet_bulb_zero_levels(snd, pmin, alloc);

        Assert(fz_levels);
        Assert(fz_levels->n_levels >= 0);

#if PRINT_DEBUG
        printf("%3ld: %ld levels  ", idx, fz_levels->n_levels);
#endif

        for(size l = 0; l < fz_levels->n_levels; ++l)
        {
            SondeLevel *lvl = &fz_levels->data[l];
            Assert(lvl->p.val > 0.0);

#if PRINT_DEBUG
            printf("%.0lf hPa,  ", lvl->p.val);
#endif

        }

#if PRINT_DEBUG
        printf("\n");
#endif

    }

#if PRINT_DEBUG
        printf("\n");
#endif

    eco_arena_destroy(alloc);
#undef PRINT_DEBUG
}

void
wbz_levels_all_tests(void)
{
    wbz_levels_test("tests/example_data/2017040118Z_namm_kmso.buf");
    wbz_levels_test("tests/example_data/2017040118Z_gfs_kmso.buf");
}

/****************************************************************************************************************************
 *                                                    All Level Tests
 ***************************************************************************************************************************/
void
all_level_tests(void)
{
    printf("\t\t\tLevel tests...");

    freezing_levels_all_tests();
    wbz_levels_all_tests();

    printf("completed.\n");
}

