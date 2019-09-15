#include "driver.h"


/* "Pacman hardware" games */
extern struct GameDriver pacman_driver;
extern struct GameDriver pacmod_driver;
extern struct GameDriver namcopac_driver;
extern struct GameDriver pacmanjp_driver;
extern struct GameDriver hangly_driver;
extern struct GameDriver puckman_driver;
extern struct GameDriver piranha_driver;
extern struct GameDriver pacplus_driver;
extern struct GameDriver mspacman_driver;
extern struct GameDriver mspacatk_driver;
extern struct GameDriver crush_driver;
extern struct GameDriver eyes_driver;
extern struct GameDriver mrtnt_driver;
extern struct GameDriver ponpoko_driver;
extern struct GameDriver lizwiz_driver;
extern struct GameDriver theglob_driver;
extern struct GameDriver beastf_driver;
extern struct GameDriver jumpshot_driver;
extern struct GameDriver maketrax_driver;
extern struct GameDriver pengo_driver;
extern struct GameDriver pengo2_driver;
extern struct GameDriver pengo2u_driver;
extern struct GameDriver penta_driver;
extern struct GameDriver jrpacman_driver;

/* "Galaxian hardware" games */
extern struct GameDriver galaxian_driver;
extern struct GameDriver galmidw_driver;
extern struct GameDriver galnamco_driver;
extern struct GameDriver superg_driver;
extern struct GameDriver galapx_driver;
extern struct GameDriver galap1_driver;
extern struct GameDriver galap4_driver;
extern struct GameDriver galturbo_driver;
extern struct GameDriver pisces_driver;
extern struct GameDriver japirem_driver;
extern struct GameDriver uniwars_driver;
extern struct GameDriver warofbug_driver;
extern struct GameDriver redufo_driver;
extern struct GameDriver pacmanbl_driver;
extern struct GameDriver zigzag_driver;
extern struct GameDriver zigzag2_driver;
extern struct GameDriver mooncrgx_driver;
extern struct GameDriver mooncrst_driver;
extern struct GameDriver mooncrsg_driver;
extern struct GameDriver mooncrsb_driver;
extern struct GameDriver fantazia_driver;
extern struct GameDriver eagle_driver;
extern struct GameDriver moonqsr_driver;
extern struct GameDriver checkman_driver;
extern struct GameDriver moonal2_driver;
extern struct GameDriver moonal2b_driver;
extern struct GameDriver kingball_driver;

/* "Scramble hardware" (and variations) games */
extern struct GameDriver scramble_driver;
extern struct GameDriver atlantis_driver;
extern struct GameDriver theend_driver;
extern struct GameDriver ckongs_driver;
extern struct GameDriver froggers_driver;
extern struct GameDriver amidars_driver;
extern struct GameDriver triplep_driver;
extern struct GameDriver scobra_driver;
extern struct GameDriver scobrak_driver;
extern struct GameDriver scobrab_driver;
extern struct GameDriver stratgyx_driver;
extern struct GameDriver stratgyb_driver;
extern struct GameDriver armorcar_driver;
extern struct GameDriver armorca2_driver;
extern struct GameDriver moonwar2_driver;
extern struct GameDriver monwar2a_driver;
extern struct GameDriver darkplnt_driver;
extern struct GameDriver tazmania_driver;
extern struct GameDriver calipso_driver;
extern struct GameDriver anteater_driver;
extern struct GameDriver rescue_driver;
extern struct GameDriver minefld_driver;
extern struct GameDriver losttomb_driver;
extern struct GameDriver losttmbh_driver;
extern struct GameDriver superbon_driver;
extern struct GameDriver hustler_driver;
extern struct GameDriver pool_driver;
extern struct GameDriver billiard_driver;
extern struct GameDriver frogger_driver;
extern struct GameDriver frogsega_driver;
extern struct GameDriver frogger2_driver;
extern struct GameDriver amidar_driver;
extern struct GameDriver amidarjp_driver;
extern struct GameDriver amigo_driver;
extern struct GameDriver turtles_driver;
extern struct GameDriver turpin_driver;
extern struct GameDriver k600_driver;
extern struct GameDriver jumpbug_driver;
extern struct GameDriver jbugsega_driver;
extern struct GameDriver flyboy_driver;
extern struct GameDriver fastfred_driver;
extern struct GameDriver jumpcoas_driver;

/* "Crazy Climber hardware" games */
extern struct GameDriver cclimber_driver;
extern struct GameDriver ccjap_driver;
extern struct GameDriver ccboot_driver;
extern struct GameDriver ckong_driver;
extern struct GameDriver ckonga_driver;
extern struct GameDriver ckongjeu_driver;
extern struct GameDriver ckongalc_driver;
extern struct GameDriver monkeyd_driver;
extern struct GameDriver rpatrolb_driver;
extern struct GameDriver silvland_driver;
extern struct GameDriver swimmer_driver;
extern struct GameDriver swimmera_driver;
extern struct GameDriver guzzler_driver;
extern struct GameDriver friskyt_driver;
extern struct GameDriver seicross_driver;

/* "Phoenix hardware" (and variations) games */
extern struct GameDriver phoenix_driver;
extern struct GameDriver phoenixt_driver;
extern struct GameDriver phoenix3_driver;
extern struct GameDriver phoenixc_driver;
extern struct GameDriver pleiads_driver;
extern struct GameDriver pleiadce_driver;
extern struct GameDriver naughtyb_driver;
extern struct GameDriver popflame_driver;

/* Namco games */
extern struct GameDriver warpwarp_driver;
extern struct GameDriver warpwar2_driver;
extern struct GameDriver rallyx_driver;
extern struct GameDriver nrallyx_driver;
extern struct GameDriver jungler_driver;
extern struct GameDriver junglers_driver;
extern struct GameDriver locomotn_driver;
extern struct GameDriver commsega_driver;
extern struct GameDriver bosco_driver;
extern struct GameDriver bosconm_driver;
extern struct GameDriver galaga_driver;
extern struct GameDriver galagamw_driver;
extern struct GameDriver galagads_driver;
extern struct GameDriver gallag_driver;
extern struct GameDriver galagab2_driver;
extern struct GameDriver digdug_driver;
extern struct GameDriver digdugnm_driver;
extern struct GameDriver xevious_driver;
extern struct GameDriver xeviousa_driver;
extern struct GameDriver xevios_driver;
extern struct GameDriver sxevious_driver;
extern struct GameDriver superpac_driver;
extern struct GameDriver superpcn_driver;
extern struct GameDriver pacnpal_driver;
extern struct GameDriver mappy_driver;
extern struct GameDriver mappyjp_driver;
extern struct GameDriver digdug2_driver;
extern struct GameDriver todruaga_driver;
extern struct GameDriver motos_driver;
extern struct GameDriver pacland_driver;
extern struct GameDriver pacland2_driver;
extern struct GameDriver pacland3_driver;
extern struct GameDriver paclandm_driver;

/* Universal games */
extern struct GameDriver cosmica_driver;
extern struct GameDriver cheekyms_driver;
extern struct GameDriver panic_driver;
extern struct GameDriver panica_driver;
extern struct GameDriver panicger_driver;
extern struct GameDriver ladybug_driver;
extern struct GameDriver ladybugb_driver;
extern struct GameDriver snapjack_driver;
extern struct GameDriver cavenger_driver;
extern struct GameDriver mrdo_driver;
extern struct GameDriver mrdot_driver;
extern struct GameDriver mrdofix_driver;
extern struct GameDriver mrlo_driver;
extern struct GameDriver mrdu_driver;
extern struct GameDriver mrdoy_driver;
extern struct GameDriver yankeedo_driver;
extern struct GameDriver docastle_driver;
extern struct GameDriver docastl2_driver;
extern struct GameDriver dounicorn_driver;
extern struct GameDriver dowild_driver;
extern struct GameDriver jjack_driver;
extern struct GameDriver dorunrun_driver;
extern struct GameDriver spiero_driver;
extern struct GameDriver kickridr_driver;

/* Nintendo games */
extern struct GameDriver radarscp_driver;
extern struct GameDriver dkong_driver;
extern struct GameDriver dkongjp_driver;
extern struct GameDriver dkongjr_driver;
extern struct GameDriver dkngjrjp_driver;
extern struct GameDriver dkjrjp_driver;
extern struct GameDriver dkjrbl_driver;
extern struct GameDriver dkong3_driver;
extern struct GameDriver mario_driver;
extern struct GameDriver masao_driver;
extern struct GameDriver hunchy_driver;
extern struct GameDriver herocast_driver;
extern struct GameDriver popeye_driver;
extern struct GameDriver popeyebl_driver;
extern struct GameDriver punchout_driver;
extern struct GameDriver spnchout_driver;
extern struct GameDriver armwrest_driver;

/* Midway 8080 b/w games */
extern struct GameDriver seawolf_driver;
extern struct GameDriver gunfight_driver;
extern struct GameDriver tornbase_driver;
extern struct GameDriver zzzap_driver;
extern struct GameDriver maze_driver;
extern struct GameDriver boothill_driver;
extern struct GameDriver checkmat_driver;
extern struct GameDriver gmissile_driver;
extern struct GameDriver clowns_driver;
extern struct GameDriver spcenctr_driver;
extern struct GameDriver invaders_driver;
extern struct GameDriver invdelux_driver;
extern struct GameDriver invadpt2_driver;
extern struct GameDriver earthinv_driver;
extern struct GameDriver spaceatt_driver;
extern struct GameDriver sinvemag_driver;
extern struct GameDriver invrvnge_driver;
extern struct GameDriver invrvnga_driver;
extern struct GameDriver galxwars_driver;
extern struct GameDriver lrescue_driver;
extern struct GameDriver grescue_driver;
extern struct GameDriver desterth_driver;
extern struct GameDriver cosmicmo_driver;
extern struct GameDriver spaceph_driver;
extern struct GameDriver rollingc_driver;
extern struct GameDriver bandido_driver;
extern struct GameDriver ozmawars_driver;
extern struct GameDriver schaser_driver;
extern struct GameDriver lupin3_driver;
extern struct GameDriver helifire_driver;
extern struct GameDriver helifira_driver;
extern struct GameDriver spacefev_driver;
extern struct GameDriver sfeverbw_driver;
extern struct GameDriver astlaser_driver;
extern struct GameDriver intruder_driver;
extern struct GameDriver polaris_driver;
extern struct GameDriver polarisa_driver;
extern struct GameDriver lagunar_driver;
extern struct GameDriver m4_driver;
extern struct GameDriver phantom2_driver;
extern struct GameDriver dogpatch_driver;
extern struct GameDriver midwbowl_driver;
extern struct GameDriver blueshrk_driver;
extern struct GameDriver einnings_driver;
extern struct GameDriver dplay_driver;
extern struct GameDriver m79amb_driver;

/* "Midway" Z80 b/w games */
extern struct GameDriver astinvad_driver;
extern struct GameDriver kamikaze_driver;

/* Meadows S2650 games */
extern struct GameDriver lazercmd_driver;
extern struct GameDriver deadeye_driver;
extern struct GameDriver gypsyjug_driver;
extern struct GameDriver medlanes_driver;

/* Midway "Astrocade" games */
extern struct GameDriver wow_driver;
extern struct GameDriver robby_driver;
extern struct GameDriver gorf_driver;
extern struct GameDriver gorfpgm1_driver;
extern struct GameDriver seawolf2_driver;
extern struct GameDriver spacezap_driver;
extern struct GameDriver ebases_driver;

/* Bally Midway "MCR" games */
extern struct GameDriver solarfox_driver;
extern struct GameDriver kick_driver;
extern struct GameDriver kicka_driver;
extern struct GameDriver shollow_driver;
extern struct GameDriver tron_driver;
extern struct GameDriver tron2_driver;
extern struct GameDriver kroozr_driver;
extern struct GameDriver domino_driver;
extern struct GameDriver wacko_driver;
extern struct GameDriver twotiger_driver;
extern struct GameDriver journey_driver;
extern struct GameDriver tapper_driver;
extern struct GameDriver sutapper_driver;
extern struct GameDriver rbtapper_driver;
extern struct GameDriver dotron_driver;
extern struct GameDriver dotrone_driver;
extern struct GameDriver destderb_driver;
extern struct GameDriver timber_driver;
extern struct GameDriver spyhunt_driver;
extern struct GameDriver crater_driver;
extern struct GameDriver sarge_driver;
extern struct GameDriver rampage_driver;
//extern struct GameDriver powerdrv_driver;
extern struct GameDriver maxrpm_driver;
extern struct GameDriver xenophob_driver;

/* Irem games */
extern struct GameDriver mpatrol_driver;
extern struct GameDriver mpatrolw_driver;
extern struct GameDriver mranger_driver;
extern struct GameDriver yard_driver;
extern struct GameDriver vsyard_driver;
extern struct GameDriver kungfum_driver;
extern struct GameDriver kungfud_driver;
extern struct GameDriver kungfub_driver;
extern struct GameDriver kungfub2_driver;
extern struct GameDriver travrusa_driver;
extern struct GameDriver motorace_driver;
extern struct GameDriver ldrun_driver;
extern struct GameDriver ldruna_driver;
extern struct GameDriver ldrun2p_driver;
extern struct GameDriver kidniki_driver;
extern struct GameDriver spelunk2_driver;
extern struct GameDriver vigilant_driver;
extern struct GameDriver vigilntj_driver;

/* Gottlieb/Mylstar games (Gottlieb became Mylstar in 1983) */
extern struct GameDriver reactor_driver;
extern struct GameDriver mplanets_driver;
extern struct GameDriver qbert_driver;
extern struct GameDriver qbertjp_driver;
extern struct GameDriver sqbert_driver;
extern struct GameDriver krull_driver;
extern struct GameDriver mach3_driver;
extern struct GameDriver usvsthem_driver;
extern struct GameDriver stooges_driver;
extern struct GameDriver qbertqub_driver;
extern struct GameDriver curvebal_driver;

/* older Taito games */
extern struct GameDriver crbaloon_driver;
extern struct GameDriver crbalon2_driver;

/* Taito "Qix hardware" games */
extern struct GameDriver qix_driver;
extern struct GameDriver qix2_driver;
extern struct GameDriver sdungeon_driver;
extern struct GameDriver zookeep_driver;
extern struct GameDriver zookeepa_driver;
extern struct GameDriver elecyoyo_driver;
extern struct GameDriver elecyoy2_driver;
extern struct GameDriver kram_driver;
extern struct GameDriver kram2_driver;

/* Taito SJ System games */
extern struct GameDriver spaceskr_driver;
extern struct GameDriver junglek_driver;
extern struct GameDriver jhunt_driver;
extern struct GameDriver alpine_driver;
extern struct GameDriver alpinea_driver;
extern struct GameDriver timetunl_driver;
extern struct GameDriver wwestern_driver;
extern struct GameDriver frontlin_driver;
extern struct GameDriver elevator_driver;
extern struct GameDriver elevatob_driver;
extern struct GameDriver tinstar_driver;
extern struct GameDriver waterski_driver;
extern struct GameDriver bioatack_driver;
extern struct GameDriver sfposeid_driver;

/* other Taito games */
extern struct GameDriver bking2_driver;
extern struct GameDriver gsword_driver;
extern struct GameDriver gladiatr_driver;
extern struct GameDriver ogonsiro_driver;
extern struct GameDriver gcastle_driver;
extern struct GameDriver tokio_driver;
extern struct GameDriver tokiob_driver;
extern struct GameDriver bublbobl_driver;
extern struct GameDriver boblbobl_driver;
extern struct GameDriver sboblbob_driver;
extern struct GameDriver rastan_driver;
extern struct GameDriver rastsaga_driver;
extern struct GameDriver rainbow_driver;
extern struct GameDriver rainbowe_driver;
extern struct GameDriver jumping_driver;
extern struct GameDriver arkanoid_driver;
extern struct GameDriver arknoidu_driver;
extern struct GameDriver arkbl2_driver;
extern struct GameDriver arkatayt_driver;
extern struct GameDriver superqix_driver;
extern struct GameDriver sqixbl_driver;
extern struct GameDriver twincobr_driver;
extern struct GameDriver twincobu_driver;
extern struct GameDriver ktiger_driver;
extern struct GameDriver arkanoi2_driver;
extern struct GameDriver ark2us_driver;
extern struct GameDriver tnzs_driver;
extern struct GameDriver tnzs2_driver;
extern struct GameDriver tigerh_driver;
extern struct GameDriver tigerh2_driver;
extern struct GameDriver tigerhb1_driver;
extern struct GameDriver tigerhb2_driver;
extern struct GameDriver slapfigh_driver;
extern struct GameDriver slapbtjp_driver;
extern struct GameDriver slapbtuk_driver;
extern struct GameDriver getstar_driver;
extern struct GameDriver superman_driver;

/* Taito F2 games */
extern struct GameDriver ssi_driver;
extern struct GameDriver liquidk_driver;
extern struct GameDriver growl_driver;

/* Williams games */
extern struct GameDriver robotron_driver;
extern struct GameDriver robotryo_driver;
extern struct GameDriver stargate_driver;
extern struct GameDriver joust_driver;
extern struct GameDriver joustr_driver;
extern struct GameDriver joustwr_driver;
extern struct GameDriver sinistar_driver;
extern struct GameDriver sinista1_driver;
extern struct GameDriver sinista2_driver;
extern struct GameDriver bubbles_driver;
extern struct GameDriver bubblesr_driver;
extern struct GameDriver defender_driver;
extern struct GameDriver splat_driver;
extern struct GameDriver blaster_driver;
extern struct GameDriver colony7_driver;
extern struct GameDriver colony7a_driver;
extern struct GameDriver lottofun_driver;
extern struct GameDriver defcmnd_driver;
extern struct GameDriver defence_driver;

/* Capcom games */
extern struct GameDriver vulgus_driver;
extern struct GameDriver vulgus2_driver;
extern struct GameDriver sonson_driver;
extern struct GameDriver higemaru_driver;
extern struct GameDriver c1942_driver;
extern struct GameDriver c1942a_driver;
extern struct GameDriver c1942b_driver;
extern struct GameDriver exedexes_driver;
extern struct GameDriver savgbees_driver;
extern struct GameDriver commando_driver;
extern struct GameDriver commandu_driver;
extern struct GameDriver commandj_driver;
extern struct GameDriver gng_driver;
extern struct GameDriver gngt_driver;
extern struct GameDriver gngcross_driver;
extern struct GameDriver gngjap_driver;
extern struct GameDriver diamond_driver;
extern struct GameDriver gunsmoke_driver;
extern struct GameDriver gunsmrom_driver;
extern struct GameDriver gunsmokj_driver;
extern struct GameDriver gunsmoka_driver;
extern struct GameDriver sectionz_driver;
extern struct GameDriver trojan_driver;
extern struct GameDriver trojanj_driver;
extern struct GameDriver srumbler_driver;
extern struct GameDriver srumblr2_driver;
extern struct GameDriver lwings_driver;
extern struct GameDriver lwings2_driver;
extern struct GameDriver lwingsjp_driver;
extern struct GameDriver sidearms_driver;
extern struct GameDriver sidearmr_driver;
extern struct GameDriver sidearjp_driver;
extern struct GameDriver avengers_driver;
extern struct GameDriver avenger2_driver;
extern struct GameDriver bionicc_driver;
extern struct GameDriver bionicc2_driver;
extern struct GameDriver c1943_driver;
extern struct GameDriver c1943jap_driver;
extern struct GameDriver blktiger_driver;
extern struct GameDriver bktigerb_driver;
extern struct GameDriver blkdrgon_driver;
extern struct GameDriver tigeroad_driver;
extern struct GameDriver f1dream_driver;
extern struct GameDriver f1dreamb_driver;
extern struct GameDriver c1943kai_driver;
extern struct GameDriver lastduel_driver;
extern struct GameDriver lstduelb_driver;
extern struct GameDriver ghouls_driver;
extern struct GameDriver ghoulsj_driver;
extern struct GameDriver madgear_driver;
extern struct GameDriver strider_driver;
extern struct GameDriver striderj_driver;
extern struct GameDriver dwj_driver;
extern struct GameDriver willow_driver;
extern struct GameDriver willowj_driver;
extern struct GameDriver unsquad_driver;
extern struct GameDriver area88_driver;
extern struct GameDriver ffight_driver;
extern struct GameDriver ffightj_driver;
extern struct GameDriver c1941_driver;
extern struct GameDriver c1941j_driver;
extern struct GameDriver mercs_driver;
extern struct GameDriver mercsu_driver;
extern struct GameDriver mercsj_driver;
extern struct GameDriver mtwins_driver;
extern struct GameDriver chikij_driver;
extern struct GameDriver msword_driver;
extern struct GameDriver mswordu_driver;
extern struct GameDriver mswordj_driver;
extern struct GameDriver cawing_driver;
extern struct GameDriver cawingj_driver;
extern struct GameDriver nemo_driver;
extern struct GameDriver nemoj_driver;
extern struct GameDriver sf2_driver;
extern struct GameDriver sf2j_driver;
extern struct GameDriver kod_driver;
extern struct GameDriver kodb_driver;
extern struct GameDriver captcomm_driver;
extern struct GameDriver knights_driver;
extern struct GameDriver varth_driver;
extern struct GameDriver pnickj_driver;
extern struct GameDriver megaman_driver;
extern struct GameDriver rockmanj_driver;

/* "Capcom Bowling hardware" games */
extern struct GameDriver capbowl_driver;
extern struct GameDriver clbowl_driver;
extern struct GameDriver bowlrama_driver;

/* Mitchell Corp games */
extern struct GameDriver pang_driver;
//extern struct GameDriver bbros_driver;
//extern struct GameDriver spang_driver;
//extern struct GameDriver block_driver;

/* Gremlin 8080 games */
extern struct GameDriver blockade_driver;
extern struct GameDriver comotion_driver;
extern struct GameDriver hustle_driver;
extern struct GameDriver blasto_driver;

/* Gremlin/Sega "VIC dual game board" games */
extern struct GameDriver depthch_driver;
extern struct GameDriver safari_driver;
extern struct GameDriver frogs_driver;
extern struct GameDriver sspaceat_driver;
extern struct GameDriver headon_driver;
extern struct GameDriver invho2_driver;
extern struct GameDriver samurai_driver;
extern struct GameDriver invinco_driver;
extern struct GameDriver invds_driver;
extern struct GameDriver tranqgun_driver;
extern struct GameDriver spacetrk_driver;
extern struct GameDriver sptrekct_driver;
extern struct GameDriver carnival_driver;
extern struct GameDriver pulsar_driver;
extern struct GameDriver heiankyo_driver;

/* Sega G-80 vector games */
extern struct GameDriver spacfury_driver;
extern struct GameDriver spacfura_driver;
extern struct GameDriver zektor_driver;
extern struct GameDriver tacscan_driver;
extern struct GameDriver elim2_driver;
extern struct GameDriver elim4_driver;
extern struct GameDriver startrek_driver;

/* Sega G-80 raster games */
extern struct GameDriver astrob_driver;
extern struct GameDriver astrob1_driver;
extern struct GameDriver s005_driver;
extern struct GameDriver monsterb_driver;
extern struct GameDriver spaceod_driver;

/* Sega "Zaxxon hardware" games */
extern struct GameDriver zaxxon_driver;
extern struct GameDriver szaxxon_driver;
extern struct GameDriver futspy_driver;
extern struct GameDriver congo_driver;
extern struct GameDriver tiptop_driver;

/* Sega System 8 games */
extern struct GameDriver starjack_driver;
extern struct GameDriver starjacs_driver;
extern struct GameDriver regulus_driver;
extern struct GameDriver regulusu_driver;
extern struct GameDriver upndown_driver;
extern struct GameDriver mrviking_driver;
extern struct GameDriver swat_driver;
extern struct GameDriver flicky_driver;
extern struct GameDriver flicky2_driver;
extern struct GameDriver bullfgtj_driver;
extern struct GameDriver pitfall2_driver;
extern struct GameDriver pitfallu_driver;
extern struct GameDriver seganinj_driver;
extern struct GameDriver seganinu_driver;
extern struct GameDriver nprinces_driver;
extern struct GameDriver nprinceb_driver;
extern struct GameDriver imsorry_driver;
extern struct GameDriver imsorryj_driver;
extern struct GameDriver teddybb_driver;
extern struct GameDriver hvymetal_driver;
extern struct GameDriver myhero_driver;
extern struct GameDriver myheroj_driver;
extern struct GameDriver chplft_driver;
extern struct GameDriver chplftb_driver;
extern struct GameDriver chplftbl_driver;
extern struct GameDriver fdwarrio_driver;
extern struct GameDriver brain_driver;
extern struct GameDriver wboy_driver;
extern struct GameDriver wboy2_driver;
extern struct GameDriver wboy3_driver;
extern struct GameDriver wboy4_driver;
extern struct GameDriver wboyu_driver;
extern struct GameDriver wboy4u_driver;
extern struct GameDriver wbdeluxe_driver;
extern struct GameDriver gardia_driver;
extern struct GameDriver blockgal_driver;
extern struct GameDriver tokisens_driver;
extern struct GameDriver dakkochn_driver;
extern struct GameDriver ufosensi_driver;
extern struct GameDriver wbml_driver;

/* Sega System 16 games */
extern struct GameDriver alexkidd_driver;
extern struct GameDriver aliensyn_driver;
extern struct GameDriver altbeast_driver;
extern struct GameDriver astormbl_driver;
extern struct GameDriver aurail_driver;
extern struct GameDriver dduxbl_driver;
extern struct GameDriver eswatbl_driver;
extern struct GameDriver fantzone_driver;
extern struct GameDriver fpointbl_driver;
extern struct GameDriver goldnaxe_driver;
extern struct GameDriver hwchamp_driver;
extern struct GameDriver mjleague_driver;
extern struct GameDriver passshtb_driver;
extern struct GameDriver quartet2_driver;
extern struct GameDriver sdi_driver;
extern struct GameDriver shinobi_driver;
extern struct GameDriver tetrisbl_driver;
extern struct GameDriver timscanr_driver;
extern struct GameDriver tturfbl_driver;
extern struct GameDriver wb3bl_driver;
extern struct GameDriver wrestwar_driver;

/* Data East "Burger Time hardware" games */
extern struct GameDriver lnc_driver;
extern struct GameDriver zoar_driver;
extern struct GameDriver btime_driver;
extern struct GameDriver btimea_driver;
extern struct GameDriver cookrace_driver;
extern struct GameDriver bnj_driver;
extern struct GameDriver brubber_driver;
extern struct GameDriver caractn_driver;
extern struct GameDriver eggs_driver;
extern struct GameDriver scregg_driver;
extern struct GameDriver tagteam_driver;

/* other Data East games */
extern struct GameDriver astrof_driver;
extern struct GameDriver astrof2_driver;
extern struct GameDriver astrof3_driver;
extern struct GameDriver tomahawk_driver;
extern struct GameDriver tomahaw5_driver;
extern struct GameDriver kchamp_driver;
extern struct GameDriver kchampvs_driver;
extern struct GameDriver karatedo_driver;
extern struct GameDriver firetrap_driver;
extern struct GameDriver firetpbl_driver;
extern struct GameDriver brkthru_driver;
extern struct GameDriver darwin_driver;
extern struct GameDriver shootout_driver;
extern struct GameDriver sidepckt_driver;
extern struct GameDriver exprraid_driver;
extern struct GameDriver wexpress_driver;
extern struct GameDriver wexpresb_driver;

/* Data East 8-bit games */
extern struct GameDriver ghostb_driver;
extern struct GameDriver mazeh_driver;
//extern struct GameDriver srdarwin_driver;
extern struct GameDriver cobracom_driver;
//extern struct GameDriver gondo_driver;
extern struct GameDriver oscar_driver;
extern struct GameDriver oscarj_driver;
//extern struct GameDriver lastmiss_driver;
//extern struct GameDriver shackled_driver;

/* Data East 16-bit games */
extern struct GameDriver karnov_driver;
extern struct GameDriver karnovj_driver;
extern struct GameDriver chelnov_driver;
extern struct GameDriver chelnovj_driver;
extern struct GameDriver hbarrel_driver;
extern struct GameDriver hbarrelj_driver;
extern struct GameDriver baddudes_driver;
extern struct GameDriver drgninja_driver;
extern struct GameDriver robocopp_driver;
extern struct GameDriver hippodrm_driver;
extern struct GameDriver ffantasy_driver;
extern struct GameDriver slyspy_driver;
extern struct GameDriver midres_driver;
extern struct GameDriver midresj_driver;
extern struct GameDriver darkseal_driver;
extern struct GameDriver gatedoom_driver;

/* Tehkan / Tecmo games (Tehkan became Tecmo in 1986) */
extern struct GameDriver bombjack_driver;
extern struct GameDriver starforc_driver;
extern struct GameDriver megaforc_driver;
extern struct GameDriver pbaction_driver;
extern struct GameDriver pbactio2_driver;
extern struct GameDriver tehkanwc_driver;
extern struct GameDriver gridiron_driver;
extern struct GameDriver teedoff_driver;
extern struct GameDriver solomon_driver;
extern struct GameDriver rygar_driver;
extern struct GameDriver rygar2_driver;
extern struct GameDriver rygarj_driver;
extern struct GameDriver gemini_driver;
extern struct GameDriver silkworm_driver;
extern struct GameDriver silkwrm2_driver;
extern struct GameDriver gaiden_driver;
extern struct GameDriver shadoww_driver;
extern struct GameDriver tknight_driver;
extern struct GameDriver wc90_driver;
extern struct GameDriver wc90b_driver;

/* Konami games */
extern struct GameDriver pooyan_driver;
extern struct GameDriver pooyans_driver;
extern struct GameDriver pootan_driver;
extern struct GameDriver timeplt_driver;
extern struct GameDriver timepltc_driver;
extern struct GameDriver spaceplt_driver;
extern struct GameDriver rocnrope_driver;
extern struct GameDriver ropeman_driver;
extern struct GameDriver gyruss_driver;
extern struct GameDriver gyrussce_driver;
extern struct GameDriver venus_driver;
extern struct GameDriver trackfld_driver;
extern struct GameDriver trackflc_driver;
extern struct GameDriver hyprolym_driver;
extern struct GameDriver hyprolyb_driver;
extern struct GameDriver circusc_driver;
extern struct GameDriver circusc2_driver;
extern struct GameDriver tp84_driver;
extern struct GameDriver tp84a_driver;
extern struct GameDriver hyperspt_driver;
extern struct GameDriver sbasketb_driver;
extern struct GameDriver mikie_driver;
extern struct GameDriver mikiej_driver;
extern struct GameDriver mikiehs_driver;
extern struct GameDriver roadf_driver;
extern struct GameDriver roadf2_driver;
extern struct GameDriver yiear_driver;
extern struct GameDriver kicker_driver;
extern struct GameDriver shaolins_driver;
extern struct GameDriver pingpong_driver;
extern struct GameDriver gberet_driver;
extern struct GameDriver rushatck_driver;
extern struct GameDriver jailbrek_driver;
extern struct GameDriver ironhors_driver;
extern struct GameDriver farwest_driver;
extern struct GameDriver jackal_driver;
extern struct GameDriver topgunr_driver;
extern struct GameDriver topgunbl_driver;
extern struct GameDriver contra_driver;
extern struct GameDriver contrab_driver;
extern struct GameDriver gryzorb_driver;
extern struct GameDriver mainevt_driver;
extern struct GameDriver devstors_driver;

/* Konami "Nemesis hardware" games */
extern struct GameDriver nemesis_driver;
extern struct GameDriver nemesuk_driver;
extern struct GameDriver konamigt_driver;
extern struct GameDriver salamand_driver;
/* GX400 BIOS based games */
extern struct GameDriver rf2_driver;
extern struct GameDriver twinbee_driver;
extern struct GameDriver gradius_driver;
extern struct GameDriver gwarrior_driver;

/* Konami "TMNT hardware" games */
extern struct GameDriver tmnt_driver;
extern struct GameDriver tmntj_driver;
extern struct GameDriver tmht2p_driver;
extern struct GameDriver tmnt2pj_driver;
extern struct GameDriver punkshot_driver;

/* Exidy games */
extern struct GameDriver sidetrac_driver;
extern struct GameDriver targ_driver;
extern struct GameDriver spectar_driver;
extern struct GameDriver spectar1_driver;
extern struct GameDriver venture_driver;
extern struct GameDriver venture2_driver;
extern struct GameDriver venture4_driver;
extern struct GameDriver mtrap_driver;
extern struct GameDriver pepper2_driver;
extern struct GameDriver hardhat_driver;
extern struct GameDriver fax_driver;
extern struct GameDriver circus_driver;
extern struct GameDriver robotbwl_driver;
extern struct GameDriver crash_driver;
extern struct GameDriver ripcord_driver;
extern struct GameDriver starfire_driver;

/* Atari vector games */
extern struct GameDriver asteroid_driver;
extern struct GameDriver asteroi1_driver;
extern struct GameDriver astdelux_driver;
//extern struct GameDriver astdelu1_driver;
extern struct GameDriver bwidow_driver;
extern struct GameDriver bzone_driver;
extern struct GameDriver bzone2_driver;
extern struct GameDriver gravitar_driver;
extern struct GameDriver llander_driver;
extern struct GameDriver llander1_driver;
extern struct GameDriver redbaron_driver;
extern struct GameDriver spacduel_driver;
extern struct GameDriver tempest_driver;
extern struct GameDriver tempest1_driver;
extern struct GameDriver tempest2_driver;
extern struct GameDriver temptube_driver;
extern struct GameDriver starwars_driver;
extern struct GameDriver empire_driver;
extern struct GameDriver mhavoc_driver;
extern struct GameDriver mhavoc2_driver;
extern struct GameDriver mhavocrv_driver;
extern struct GameDriver quantum_driver;
extern struct GameDriver quantum1_driver;

/* Atari "Centipede hardware" games */
extern struct GameDriver warlord_driver;
extern struct GameDriver centiped_driver;
extern struct GameDriver centipd2_driver;
extern struct GameDriver milliped_driver;
extern struct GameDriver qwakprot_driver;

/* Atari "Kangaroo hardware" games */
extern struct GameDriver kangaroo_driver;
extern struct GameDriver kangarob_driver;
extern struct GameDriver arabian_driver;

/* Atari "Missile Command hardware" games */
extern struct GameDriver missile_driver;
extern struct GameDriver missile2_driver;
extern struct GameDriver suprmatk_driver;

/* Atari b/w games */
extern struct GameDriver sprint1_driver;
extern struct GameDriver sprint2_driver;
extern struct GameDriver sbrkout_driver;
extern struct GameDriver dominos_driver;
extern struct GameDriver nitedrvr_driver;
extern struct GameDriver bsktball_driver;
extern struct GameDriver copsnrob_driver;
extern struct GameDriver avalnche_driver;
extern struct GameDriver subs_driver;

/* Atari System 1 games */
extern struct GameDriver marble_driver;
extern struct GameDriver marble2_driver;
extern struct GameDriver marblea_driver;
extern struct GameDriver peterpak_driver;
extern struct GameDriver indytemp_driver;
extern struct GameDriver roadrunn_driver;
extern struct GameDriver roadblst_driver;

/* Atari System 2 games */
extern struct GameDriver paperboy_driver;
extern struct GameDriver ssprint_driver;
extern struct GameDriver csprint_driver;
extern struct GameDriver a720_driver;
extern struct GameDriver a720b_driver;
extern struct GameDriver apb_driver;
extern struct GameDriver apb2_driver;

/* later Atari games */
extern struct GameDriver gauntlet_driver;
extern struct GameDriver gauntir1_driver;
extern struct GameDriver gauntir2_driver;
extern struct GameDriver gaunt2p_driver;
extern struct GameDriver gaunt2_driver;
extern struct GameDriver atetris_driver;
extern struct GameDriver atetrisa_driver;
extern struct GameDriver atetrisb_driver;
extern struct GameDriver atetcktl_driver;
extern struct GameDriver atetckt2_driver;
extern struct GameDriver toobin_driver;
extern struct GameDriver vindictr_driver;
extern struct GameDriver klax_driver;
extern struct GameDriver klax2_driver;
extern struct GameDriver klax3_driver;
extern struct GameDriver blstroid_driver;
extern struct GameDriver eprom_driver;
extern struct GameDriver xybots_driver;

/* SNK / Rock-ola games */
extern struct GameDriver sasuke_driver;
extern struct GameDriver satansat_driver;
extern struct GameDriver zarzon_driver;
extern struct GameDriver vanguard_driver;
extern struct GameDriver vangrdce_driver;
extern struct GameDriver fantasy_driver;
extern struct GameDriver pballoon_driver;
extern struct GameDriver nibbler_driver;
extern struct GameDriver nibblera_driver;

/* Technos games */
extern struct GameDriver mystston_driver;
extern struct GameDriver matmania_driver;
extern struct GameDriver excthour_driver;
extern struct GameDriver maniach_driver;
extern struct GameDriver maniach2_driver;
extern struct GameDriver renegade_driver;
extern struct GameDriver kuniokub_driver;
extern struct GameDriver xsleena_driver;
extern struct GameDriver solarwar_driver;
extern struct GameDriver ddragon_driver;
extern struct GameDriver ddragonb_driver;
extern struct GameDriver ddragon2_driver;
extern struct GameDriver blockout_driver;

/* Stern "Berzerk hardware" games */
extern struct GameDriver berzerk_driver;
extern struct GameDriver berzerk1_driver;
extern struct GameDriver frenzy_driver;
extern struct GameDriver frenzy1_driver;

/* GamePlan games */
extern struct GameDriver megatack_driver;
extern struct GameDriver killcom_driver;
extern struct GameDriver challeng_driver;
extern struct GameDriver kaos_driver;

/* "stratovox hardware" games */
extern struct GameDriver route16_driver;
extern struct GameDriver stratvox_driver;
extern struct GameDriver speakres_driver;

/* Zaccaria games */
extern struct GameDriver monymony_driver;
extern struct GameDriver jackrabt_driver;
extern struct GameDriver jackrab2_driver;
extern struct GameDriver jackrabs_driver;

/* UPL games */
extern struct GameDriver nova2001_driver;
extern struct GameDriver pkunwar_driver;
extern struct GameDriver pkunwarj_driver;
extern struct GameDriver ninjakd2_driver;
extern struct GameDriver ninjak2a_driver;

/* Williams/Midway TMS34010 games */
extern struct GameDriver narc_driver;
extern struct GameDriver trog_driver;
extern struct GameDriver trog3_driver;
extern struct GameDriver trogp_driver;
extern struct GameDriver smashtv_driver;
extern struct GameDriver smashtv5_driver;
extern struct GameDriver hiimpact_driver;
extern struct GameDriver shimpact_driver;
extern struct GameDriver strkforc_driver;
extern struct GameDriver mk_driver;
extern struct GameDriver term2_driver;
extern struct GameDriver totcarn_driver;
extern struct GameDriver totcarnp_driver;
extern struct GameDriver mk2_driver;
extern struct GameDriver nbajam_driver;

/* Cinematronics raster games */
extern struct GameDriver jack_driver;
extern struct GameDriver jacka_driver;
extern struct GameDriver treahunt_driver;
extern struct GameDriver zzyzzyxx_driver;
extern struct GameDriver brix_driver;
extern struct GameDriver sucasino_driver;

/* "The Pit hardware" games */
extern struct GameDriver roundup_driver;
extern struct GameDriver thepit_driver;
extern struct GameDriver intrepid_driver;
extern struct GameDriver portman_driver;
extern struct GameDriver suprmous_driver;

/* Valadon Automation games */
extern struct GameDriver bagman_driver;
extern struct GameDriver bagmans_driver;
extern struct GameDriver bagmans2_driver;
extern struct GameDriver sbagman_driver;
extern struct GameDriver sbagmans_driver;
extern struct GameDriver pickin_driver;

/* Seibu Denshi / Seibu Kaihatsu games */
extern struct GameDriver stinger_driver;
extern struct GameDriver scion_driver;
extern struct GameDriver scionc_driver;
extern struct GameDriver wiz_driver;

/* Nichibutsu games */
extern struct GameDriver cop01_driver;
extern struct GameDriver cop01a_driver;
extern struct GameDriver terracre_driver;
extern struct GameDriver terracra_driver;
extern struct GameDriver galivan_driver;

/* Neo Geo games */
extern struct GameDriver joyjoy_driver;
extern struct GameDriver ridhero_driver;
extern struct GameDriver bstars_driver;
extern struct GameDriver lbowling_driver;
extern struct GameDriver superspy_driver;
extern struct GameDriver ttbb_driver;
extern struct GameDriver alpham2_driver;
extern struct GameDriver eightman_driver;
extern struct GameDriver ararmy_driver;
extern struct GameDriver fatfury1_driver;
extern struct GameDriver burningf_driver;
extern struct GameDriver kingofm_driver;
extern struct GameDriver gpilots_driver;
extern struct GameDriver lresort_driver;
extern struct GameDriver fbfrenzy_driver;
extern struct GameDriver socbrawl_driver;
extern struct GameDriver mutnat_driver;
extern struct GameDriver artfight_driver;
extern struct GameDriver countb_driver;
extern struct GameDriver ncombat_driver;
extern struct GameDriver crsword_driver;
extern struct GameDriver trally_driver;
extern struct GameDriver sengoku_driver;
extern struct GameDriver ncommand_driver;
extern struct GameDriver wheroes_driver;
extern struct GameDriver sengoku2_driver;
extern struct GameDriver androdun_driver;
extern struct GameDriver bjourney_driver;
extern struct GameDriver maglord_driver;
extern struct GameDriver janshin_driver;
extern struct GameDriver pulstar_driver;
extern struct GameDriver blazstar_driver;
extern struct GameDriver pbobble_driver;
extern struct GameDriver puzzledp_driver;
extern struct GameDriver pspikes2_driver;
extern struct GameDriver sonicwi2_driver;
extern struct GameDriver sonicwi3_driver;
extern struct GameDriver goalx3_driver;
extern struct GameDriver neodrift_driver;
extern struct GameDriver neomrdo_driver;
extern struct GameDriver spinmast_driver;
extern struct GameDriver karnov_r_driver;
extern struct GameDriver wjammers_driver;
extern struct GameDriver strhoops_driver;
extern struct GameDriver magdrop2_driver;
extern struct GameDriver magdrop3_driver;
extern struct GameDriver mslug_driver;
extern struct GameDriver turfmast_driver;
extern struct GameDriver kabukikl_driver;
extern struct GameDriver panicbom_driver;
extern struct GameDriver neobombe_driver;
extern struct GameDriver worlher2_driver;
extern struct GameDriver worlhe2j_driver;
extern struct GameDriver aodk_driver;
extern struct GameDriver whp_driver;
extern struct GameDriver ninjamas_driver;
extern struct GameDriver overtop_driver;
extern struct GameDriver twinspri_driver;
extern struct GameDriver stakwin_driver;
extern struct GameDriver ragnagrd_driver;
extern struct GameDriver shocktro_driver;
extern struct GameDriver tws96_driver;
extern struct GameDriver zedblade_driver;
extern struct GameDriver doubledr_driver;
extern struct GameDriver gowcaizr_driver;
extern struct GameDriver galaxyfg_driver;
extern struct GameDriver wakuwak7_driver;
extern struct GameDriver viewpoin_driver;
extern struct GameDriver gururin_driver;
extern struct GameDriver miexchng_driver;
extern struct GameDriver mahretsu_driver;
extern struct GameDriver nam_1975_driver;
extern struct GameDriver cyberlip_driver;
extern struct GameDriver tpgolf_driver;
extern struct GameDriver legendos_driver;
extern struct GameDriver fatfury2_driver;
extern struct GameDriver bstars2_driver;
extern struct GameDriver sidkicks_driver;
extern struct GameDriver kotm2_driver;
extern struct GameDriver samsho_driver;
extern struct GameDriver fatfursp_driver;
extern struct GameDriver fatfury3_driver;
extern struct GameDriver tophuntr_driver;
extern struct GameDriver savagere_driver;
extern struct GameDriver kof94_driver;
extern struct GameDriver aof2_driver;
extern struct GameDriver ssideki2_driver;
extern struct GameDriver sskick3_driver;
extern struct GameDriver samsho2_driver;
extern struct GameDriver samsho3_driver;
extern struct GameDriver kof95_driver;
extern struct GameDriver rbff1_driver;
extern struct GameDriver aof3_driver;
extern struct GameDriver kof96_driver;
extern struct GameDriver samsho4_driver;
extern struct GameDriver rbffspec_driver;
extern struct GameDriver kizuna_driver;
extern struct GameDriver kof97_driver;
extern struct GameDriver lastblad_driver;
extern struct GameDriver mslug2_driver;
extern struct GameDriver realbou2_driver;

extern struct GameDriver spacefb_driver;
extern struct GameDriver tutankhm_driver;
extern struct GameDriver tutankst_driver;
extern struct GameDriver junofrst_driver;
extern struct GameDriver ccastles_driver;
extern struct GameDriver ccastle2_driver;
extern struct GameDriver blueprnt_driver;
extern struct GameDriver omegrace_driver;
extern struct GameDriver bankp_driver;
extern struct GameDriver espial_driver;
extern struct GameDriver espiale_driver;
extern struct GameDriver cloak_driver;
extern struct GameDriver champbas_driver;
extern struct GameDriver champbb2_driver;
//extern struct GameDriver sinbadm_driver;
extern struct GameDriver exerion_driver;
extern struct GameDriver exeriont_driver;
extern struct GameDriver exerionb_driver;
extern struct GameDriver foodf_driver;
extern struct GameDriver vastar_driver;
extern struct GameDriver vastar2_driver;
extern struct GameDriver formatz_driver;
extern struct GameDriver aeroboto_driver;
extern struct GameDriver citycon_driver;
extern struct GameDriver psychic5_driver;
extern struct GameDriver jedi_driver;
extern struct GameDriver tankbatt_driver;
extern struct GameDriver liberatr_driver;
extern struct GameDriver dday_driver;
extern struct GameDriver toki_driver;
extern struct GameDriver snowbros_driver;
extern struct GameDriver snowbro2_driver;
extern struct GameDriver gundealr_driver;
extern struct GameDriver gundeala_driver;
extern struct GameDriver leprechn_driver;
extern struct GameDriver potogold_driver;
extern struct GameDriver hexa_driver;
extern struct GameDriver redalert_driver;
extern struct GameDriver irobot_driver;
extern struct GameDriver spiders_driver;
extern struct GameDriver stactics_driver;
extern struct GameDriver goldstar_driver;
extern struct GameDriver goldstbl_driver;
extern struct GameDriver exterm_driver;
extern struct GameDriver sharkatt_driver;
extern struct GameDriver turbo_driver;
extern struct GameDriver turboa_driver;
extern struct GameDriver turbob_driver;
extern struct GameDriver kingofb_driver;
extern struct GameDriver kingofbj_driver;
extern struct GameDriver ringking_driver;
extern struct GameDriver ringkin2_driver;
extern struct GameDriver zerozone_driver;
extern struct GameDriver exctsccr_driver;
extern struct GameDriver exctsccb_driver;
extern struct GameDriver speedbal_driver;
extern struct GameDriver sauro_driver;
extern struct GameDriver pow_driver;
extern struct GameDriver powj_driver;

//#define CLASSIC 1

const struct GameDriver *drivers[] =
{
 &mrdo_driver,   /* (c) 1982 */
  &mrdot_driver,    /* (c) 1982 + Taito license */
  &mrdofix_driver,  /* (c) 1982 + Taito license */
  &mrlo_driver,   /* bootleg */
  &mrdu_driver,   /* bootleg */
  &mrdoy_driver,    /* bootleg */
  &yankeedo_driver, /* bootleg */  
  &docastle_driver, /* (c) 1983 */
  &docastl2_driver, /* (c) 1983 */
  &dounicorn_driver,  /* (c) 1983 */
  &dowild_driver,   /* (c) 1984 */
  &jjack_driver,    /* (c) 1984 */
  &dorunrun_driver, /* (c) 1984 */
  &spiero_driver,   /* (c) 1987 */
  &kickridr_driver, /* (c) 1984 */

  /* "Phoenix hardware" (and variations) games */
  &phoenix_driver,  /* (c) 1980 Amstar */
  &phoenixt_driver, /* (c) 1980 Taito */
  &phoenix3_driver, /* bootleg */
  &phoenixc_driver, /* bootleg */
  &pleiads_driver,  /* (c) 1981 Tehkan */
  &pleiadce_driver, /* (c) 1981 Centuri + Tehkan */

  &ladybug_driver,  /* (c) 1981 */
  &ladybugb_driver, /* bootleg */
  &snapjack_driver, /* (c) */
  &cavenger_driver, /* (c) 1981 */

  &galaga_driver,   /* (c) 1981 */
  &galagamw_driver, /* (c) 1981 Midway */
  &galagads_driver, /* hack */
  &gallag_driver,   /* bootleg */
  &galagab2_driver, /* bootleg */

#ifdef CLASSIC
 /* Irem games */
  &mpatrol_driver,  /* (c) 1982 */
  &mpatrolw_driver, /* (c) 1982 + Williams license */
  &mranger_driver,  /* bootleg */

  /* Stern "Berzerk hardware" games */
  &berzerk_driver,  /* (c) 1980 */
  &berzerk1_driver, /* (c) 1980 */
  &frenzy_driver,   /* (c) 1982 */
  &frenzy1_driver,  /* (c) 1982 */

   
  &gyruss_driver,   /* GX347 (c) 1983 */
  &gyrussce_driver, /* GX347 (c) 1983 + Centuri license */
  &xevious_driver,  /* (c) 1982 */
  &xeviousa_driver, /* (c) 1982 + Atari license */
  &xevios_driver,   /* bootleg */
  &sxevious_driver, /* (c) 1984 */
	&contra_driver,		/* GX633 (c) 1987 */
	&contrab_driver,	/* bootleg */
	&gryzorb_driver,	/* bootleg */
	&commando_driver,	/* (c) 1985 */
	&commandu_driver,	/* (c) 1985 */
	&commandj_driver,	/* (c) 1985 */
	&gng_driver,		/* (c) 1985 */
	&gngt_driver,		/* (c) 1985 */
	&gngcross_driver,	/* (c) 1985 */
	&gngjap_driver,		/* (c) 1985 */
	&c1942_driver,		/* (c) 1984 */
	&c1942a_driver,		/* (c) 1984 */
	&c1942b_driver,		/* (c) 1984 */
	&bosco_driver,		/* (c) 1981 Midway */
	&bosconm_driver,	/* (c) 1981 */
#endif
#ifdef UNUSED
	&jrpacman_driver,	/* (c) 1983 Midway */
	&friskyt_driver,	/* (c) 1981 Nichibutsu */
	&seicross_driver,	/* (c) 1984 Nichibutsu + Alice */
	&ckongs_driver,		/* bootleg */
	&scobra_driver,		/* GX316 (c) 1981 Stern */
	&scobrak_driver,	/* GX316 (c) 1981 Konami */
	&scobrab_driver,	/* GX316 (c) 1981 Karateco (bootleg?) */
	&eyes_driver,		/* (c) 1982 Digitrex Techstar + "Rockola presents" */
	&mrtnt_driver,		/* (c) 1983 Telko */
	&ponpoko_driver,	/* (c) 1982 Sigma Ent. Inc. */
	&lizwiz_driver,		/* (c) 1985 Techstar + "Sunn presents" */
	&theglob_driver,	/* (c) 1983 Epos Corporation */
	&beastf_driver,		/* (c) 1984 Epos Corporation */
	&jumpshot_driver,
	&japirem_driver,	/* (c) Irem */
	&uniwars_driver,	/* (c) Karateco (bootleg?) */
	&warofbug_driver,	/* (c) 1981 Armenia */
	&redufo_driver,		/* ? */
	&pacmanbl_driver,	/* bootleg */
	&zigzag_driver,		/* (c) 1982 LAX */
	&zigzag2_driver,	/* (c) 1982 LAX */
	&fantazia_driver,	/* bootleg */
	&eagle_driver,		/* (c) Centuri */
	&moonqsr_driver,	/* (c) 1980 Nichibutsu */
	&checkman_driver,	/* (c) 1982 Zilec-Zenitone */
	&moonal2_driver,	/* Nichibutsu */
	&moonal2b_driver,	/* Nichibutsu */
	&kingball_driver,	/* no copyright notice (the char set contains Namco) */

	&stratgyx_driver,	/* (c) 1981 Stern */
	&stratgyb_driver,	/* bootleg (of the Konami version?) */
	&armorcar_driver,	/* (c) 1981 Stern */
	&armorca2_driver,	/* (c) 1981 Stern */
	&moonwar2_driver,	/* (c) 1981 Stern */
	&monwar2a_driver,	/* (c) 1981 Stern */
	&darkplnt_driver,	/* (c) 1982 Stern */
	&tazmania_driver,	/* (c) 1982 Stern */
	&calipso_driver,	/* (c) 1982 Tago */
	&anteater_driver,	/* (c) 1982 Tago */
	&rescue_driver,		/* (c) 1982 Stern */
	&minefld_driver,	/* (c) 1983 Stern */
	&losttomb_driver,	/* (c) 1982 Stern */
	&losttmbh_driver,	/* (c) 1982 Stern */
	&superbon_driver,	/* bootleg */
	&hustler_driver,	/* GX343 (c) 1981 Konami */
	&pool_driver,
	&billiard_driver,
	&k600_driver,		/* GX353 (c) 1981 Konami */
	&jumpbug_driver,	/* (c) 1981 Rock-ola */
	&jbugsega_driver,	/* (c) 1981 Sega */
	&flyboy_driver,		/* (c) 1982 Kaneko (bootleg?) */
	&fastfred_driver,	/* (c) 1982 Atari */
	&jumpcoas_driver,	/* (c) 1983 Kaneko */


	/* "Phoenix hardware" (and variations) games */
	&phoenix_driver,	/* (c) 1980 Amstar */
	&phoenixt_driver,	/* (c) 1980 Taito */
	&phoenix3_driver,	/* bootleg */
	&phoenixc_driver,	/* bootleg */
	&pleiads_driver,	/* (c) 1981 Tehkan */
	&pleiadce_driver,	/* (c) 1981 Centuri + Tehkan */
	&naughtyb_driver,	/* (c) 1982 Jaleco + Cinematronics */
	&popflame_driver,	/* (c) 1982 Jaleco */

	/* Namco games */
	&warpwarp_driver,	/* (c) 1981 Rock-ola - different hardware */
						/* the high score table says "NAMCO" */
	&warpwar2_driver,	/* (c) 1981 Rock-ola - different hardware */
						/* the high score table says "NAMCO" */
	&rallyx_driver,		/* (c) 1980 Midway */
	&nrallyx_driver,	/* (c) 1981 Namco */
	&jungler_driver,	/* GX327 (c) 1981 Konami */
	&junglers_driver,	/* GX327 (c) 1981 Stern */
	&locomotn_driver,	/* GX359 (c) 1982 Konami + Centuri license */
	&commsega_driver,	/* (c) 1983 Sega */
	/* the following ones all have a custom I/O chip */
	&superpac_driver,	/* (c) 1982 Midway */
	&superpcn_driver,	/* (c) 1982 */
	&pacnpal_driver,	/* (c) 1983 */
	&mappy_driver,		/* (c) 1983 */
	&mappyjp_driver,	/* (c) 1983 */
	&digdug2_driver,	/* (c) 1985 */
	&todruaga_driver,	/* (c) 1984 */
	&motos_driver,		/* (c) 1985 */
	/* no custom I/O in the following, HD63701 (or compatible) microcontroller instead */
	&pacland_driver,	/* (c) 1984 */
	&pacland2_driver,	/* (c) 1984 */
	&pacland3_driver,	/* (c) 1984 */
	&paclandm_driver,	/* (c) 1984 Midway */

	/* Universal games */
	&cosmica_driver,	/* (c) [1979] */
	&cheekyms_driver,	/* (c) [1980?] */
	&panic_driver,		/* (c) 1980 */
	&panica_driver,		/* (c) 1980 */
	&panicger_driver,	/* (c) 1980 */

	&masao_driver,		/* bootleg */
//	&hunchy_driver,		/* hacked Donkey Kong board */
//	&herocast_driver,
	&popeye_driver,
	&popeyebl_driver,	/* bootleg */
	&punchout_driver,	/* (c) 1984 */
	&spnchout_driver,	/* (c) 1984 */
	&armwrest_driver,	/* (c) 1985 */

	/* Midway 8080 b/w games */
	&seawolf_driver,	/* 596 [1976] */
	&gunfight_driver,	/* 597 [1975] */
	/* 603 - Top Gun [1976] */
	&tornbase_driver,	/* 605 [1976] */
	&zzzap_driver,		/* 610 [1976] */
	&maze_driver,		/* 611 [1976] */
	&boothill_driver,	/* 612 [1977] */
	&checkmat_driver,	/* 615 [1977] */
	/* 618 - Road Runner [1977] */
	/* 618 - Desert Gun [1977] */
	&dplay_driver,		/* 619 [1977] */
	&lagunar_driver,	/* 622 [1977] */
	&gmissile_driver,	/* 623 [1977] */
	&m4_driver,			/* 626 [1977] */
	&clowns_driver,		/* 630 [1978] */
	/* 640 - Space Walk [1978] */
	&einnings_driver,	/* 642 [1978] Midway */
	/* 643 - Shuffleboard [1978] */
	&dogpatch_driver,	/* 644 [1977] */
	&spcenctr_driver,	/* 645 (c) 1980 Midway */
	&phantom2_driver,	/* 652 [1979] */
	&midwbowl_driver,	/* 730 [1978] Midway */
	&invaders_driver,	/* 739 [1979] */
	&blueshrk_driver,	/* 742 [1978] */
	&invdelux_driver,	/* 852 [1980] Midway */
	&invadpt2_driver,	/* 851 [1980] Taito */
	&earthinv_driver,
	&spaceatt_driver,
	&sinvemag_driver,
	&invrvnge_driver,
	&invrvnga_driver,
	&galxwars_driver,
	&lrescue_driver,	/* (c) 1979 Taito */
	&grescue_driver,	/* bootleg? */
	&desterth_driver,	/* bootleg */
	&cosmicmo_driver,	/* Universal */
	&spaceph_driver,	/* Zilec Games */
	&rollingc_driver,	/* Nichibutsu */
	&bandido_driver,	/* (c) Exidy */
	&ozmawars_driver,	/* Shin Nihon Kikaku (SNK) */
	&schaser_driver,	/* Taito */
	&lupin3_driver,		/* (c) 1980 Taito */
	&helifire_driver,	/* (c) Nintendo */
	&helifira_driver,	/* (c) Nintendo */
	&spacefev_driver,
	&sfeverbw_driver,
	&astlaser_driver,
	&intruder_driver,
	&polaris_driver,	/* (c) 1980 Taito */
	&polarisa_driver,	/* (c) 1980 Taito */
	&m79amb_driver,

	/* "Midway" Z80 b/w games */
	&astinvad_driver,	/* (c) 1980 Stern */
	&kamikaze_driver,	/* Leijac Corporation */

	/* Meadows S2650 games */
	&lazercmd_driver,	/* [1976?] */
	&deadeye_driver,	/* [1978?] */
	&gypsyjug_driver,	/* [1978?] */
	&medlanes_driver,	/* [1977?] */

	/* Midway "Astrocade" games */
	&wow_driver,		/* (c) 1980 */
	&robby_driver,		/* (c) 1981 */
	&gorf_driver,		/* (c) 1981 */
	&gorfpgm1_driver,	/* (c) 1981 */
	&seawolf2_driver,
	&spacezap_driver,	/* (c) 1980 */
	&ebases_driver,

	/* Bally Midway "MCR" games */
	/* MCR1 */
	&solarfox_driver,	/* (c) 1981 */
	&kick_driver,		/* (c) 1981 */
	&kicka_driver,		/* bootleg? */
	/* MCR2 */
	&shollow_driver,	/* (c) 1981 */
	&tron_driver,		/* (c) 1982 */
	&tron2_driver,		/* (c) 1982 */
	&kroozr_driver,		/* (c) 1982 */
	&domino_driver,		/* (c) 1982 */
	&wacko_driver,		/* (c) 1982 */
	&twotiger_driver,	/* (c) 1984 */
	/* MCR2 + MCR3 sprites */
	&journey_driver,	/* (c) 1983 */
	/* MCR3 */
	&tapper_driver,		/* (c) 1983 */
	&sutapper_driver,	/* (c) 1983 */
	&rbtapper_driver,	/* (c) 1984 */
	&dotron_driver,		/* (c) 1983 */
	&dotrone_driver,	/* (c) 1983 */
	&destderb_driver,	/* (c) 1984 */
	&timber_driver,		/* (c) 1984 */
	&spyhunt_driver,	/* (c) 1983 */
	&crater_driver,		/* (c) 1984 */
	&sarge_driver,		/* (c) 1985 */
	&rampage_driver,	/* (c) 1986 */
//	&powerdrv_driver,
	&maxrpm_driver,		/* (c) 1986 */
	/* MCR 68000 */
	&xenophob_driver,	/* (c) 1987 */
	/* Power Drive - similar to Xenophobe? */
/* other possible MCR games:
Black Belt
Shoot the Bull
Special Force
MotorDome
Six Flags (?)
*/

	&yard_driver,		/* (c) 1983 */
	&vsyard_driver,		/* (c) 1983/1984 */
	&kungfum_driver,	/* (c) 1984 */
	&kungfud_driver,	/* (c) 1984 + Data East license */
	&kungfub_driver,	/* bootleg */
	&kungfub2_driver,	/* bootleg */
	&travrusa_driver,	/* (c) 1983 */
	&motorace_driver,	/* (c) 1983 Williams license */
	&ldrun_driver,		/* (c) 1984 licensed from Broderbund */
	&ldruna_driver,		/* (c) 1984 licensed from Broderbund */
	&ldrun2p_driver,	/* (c) 1986 licensed from Broderbund */
	&kidniki_driver,	/* (c) 1986 + Data East USA license */
	&spelunk2_driver,	/* (c) 1986 licensed from Broderbund */
	&vigilant_driver,	/* (c) 1988 */
	&vigilntj_driver,	/* (c) 1988 */

	/* Gottlieb/Mylstar games (Gottlieb became Mylstar in 1983) */
	&reactor_driver,	/* GV-100 (c) 1982 Gottlieb */
	&mplanets_driver,	/* GV-102 (c) 1983 Gottlieb */
	&qbert_driver,		/* GV-103 (c) 1982 Gottlieb */
	&qbertjp_driver,	/* GV-103 (c) 1982 Gottlieb + Konami license */
	&sqbert_driver,		/* (c) 1983 Mylstar - never released */
	&krull_driver,		/* GV-105 (c) 1983 Gottlieb */
	&mach3_driver,		/* GV-109 (c) 1983 Mylstar */
	&usvsthem_driver,	/* GV-??? (c) 198? Mylstar */
	&stooges_driver,	/* GV-113 (c) 1984 Mylstar */
	&qbertqub_driver,	/* GV-119 (c) 1983 Mylstar */
	&curvebal_driver,	/* GV-134 (c) 1984 Mylstar */

	/* older Taito games */
	&crbaloon_driver,	/* (c) 1980 Taito */
	&crbalon2_driver,	/* (c) 1980 Taito */

	/* Taito "Qix hardware" games */
	&qix_driver,		/* (c) 1981 */
	&qix2_driver,		/* (c) 1981 */
	&sdungeon_driver,	/* (c) 1981 Taito America */
	&zookeep_driver,	/* (c) 1982 Taito America */
	&zookeepa_driver,	/* (c) 1982 Taito America */
	&elecyoyo_driver,
	&elecyoy2_driver,
	&kram_driver,		/* (c) 1982 Taito America */
	&kram2_driver,		/* (c) 1982 Taito America */

	/* Taito SJ System games */
	&spaceskr_driver,	/* (c) 1981 */
	&junglek_driver,	/* (c) 1982 */
	&jhunt_driver,		/* (c) 1982 Taito America */
	&alpine_driver,		/* (c) 1982 */
	&alpinea_driver,	/* (c) 1982 */
	&timetunl_driver,	/* (c) 1982 */
	&wwestern_driver,	/* (c) 1982 */
	&frontlin_driver,	/* (c) 1982 */
	&elevator_driver,	/* (c) 1983 */
	&elevatob_driver,	/* bootleg */
	&tinstar_driver,	/* (c) 1983 */
	&waterski_driver,	/* (c) 1983 */
	&bioatack_driver,	/* (c) 1983 + Fox Video Games license */
	&sfposeid_driver,	/* (c) 1984 */

	/* other Taito games */
	&bking2_driver,		/* (c) 1983 */
	&gsword_driver,		/* (c) 1984 */
	&gladiatr_driver,	/* (c) 1986 Taito America */
	&ogonsiro_driver,	/* (c) 1986 */
	&gcastle_driver,	/* (c) 1986 */
	&tokio_driver,		/* (c) 1986 */
	&tokiob_driver,		/* bootleg */
	&bublbobl_driver,	/* (c) 1986 */
	&boblbobl_driver,	/* bootleg */
	&sboblbob_driver,	/* bootleg */
	&rastan_driver,		/* (c) 1987 Taito Japan */
	&rastsaga_driver,	/* (c) 1987 Taito */
	&rainbow_driver,	/* (c) 1987 */
	&rainbowe_driver,	/* (c) 1988 */
	&jumping_driver,	/* bootleg */
	&arkanoid_driver,	/* (c) 1986 Taito */
	&arknoidu_driver,	/* (c) 1986 Taito America + Romstar license */
	&arkbl2_driver,		/* bootleg */
	&arkatayt_driver,	/* bootleg */
	&superqix_driver,	/* (c) 1987 */
	&sqixbl_driver,		/* bootleg? but (c) 1987 */
	&twincobr_driver,	/* (c) 1987 */
	&twincobu_driver,	/* (c) 1987 Taito America + Romstar license */
	&ktiger_driver,		/* (c) 1987 */
	&arkanoi2_driver,	/* (c) 1987 */
	&ark2us_driver,		/* (c) 1987 + Romstar license */
	&tnzs_driver,		/* (c) 1988 */
	&tnzs2_driver,		/* (c) 1988 */
	&tigerh_driver,		/* (c) 1985 */
	&tigerh2_driver,	/* (c) 1985 */
	&tigerhb1_driver,	/* bootleg */
	&tigerhb2_driver,	/* bootleg */
	&slapfigh_driver,	/* (c) 1988 */
	&slapbtjp_driver,	/* bootleg */
	&slapbtuk_driver,	/* bootleg */
	&getstar_driver,	/* (c) 1986 Taito, but bootleg */
	&superman_driver,	/* (c) 1988 */

	/* Taito F2 games */
	&ssi_driver,		/* (c) 1990 */
	&liquidk_driver,	/* (c) 1990 */
	&growl_driver,		/* (c) 1990 */

	/* Williams games */
	&robotron_driver,	/* (c) 1982 */
	&robotryo_driver,	/* (c) 1982 */
	&stargate_driver,	/* (c) 1981 */
	&joust_driver,		/* (c) 1982 */
	&joustr_driver,		/* (c) 1982 */
	&joustwr_driver,	/* (c) 1982 */
	&sinistar_driver,	/* (c) 1982 */
	&sinista1_driver,	/* (c) 1982 */
	&sinista2_driver,	/* (c) 1982 */
	&bubbles_driver,	/* (c) 1982 */
	&bubblesr_driver,	/* (c) 1982 */
	&defender_driver,	/* (c) 1980 */
	&splat_driver,		/* (c) 1982 */
	&blaster_driver,	/* (c) 1983 */
	&colony7_driver,	/* (c) 1981 Taito */
	&colony7a_driver,	/* (c) 1981 Taito */
	&lottofun_driver,	/* (c) 1987 H.A.R. Management */
	&defcmnd_driver,	/* bootleg */
	&defence_driver,	/* bootleg */

	/* Capcom games */
	/* The following is a COMPLETE list of the Capcom games up to 1997, as shown on */
	/* their web site. The list is sorted by production date. Some titles are in */
	/* quotes, because I couldn't find the English name (might not have been exported). */
	/* The name in quotes is the title Capcom used for the html page. */
	&vulgus_driver,		/* (c) 1984 */
	&vulgus2_driver,	/* (c) 1984 */
	&sonson_driver,		/* (c) 1984 */
	&higemaru_driver,	/* (c) 1984 */
	&exedexes_driver,	/* (c) 1985 */
	&savgbees_driver,	/* (c) 1985 + Memetron license */
	&diamond_driver,	/* (c) 1989 KH Video (NOT A CAPCOM GAME but runs on GnG hardware) */
	&gunsmoke_driver,	/* (c) 1985 */
	&gunsmrom_driver,	/* (c) 1985 + Romstar */
	&gunsmokj_driver,	/* (c) 1985 */
	&gunsmoka_driver,	/* (c) 1985 */
	&sectionz_driver,	/* (c) 1985 */
	&trojan_driver,		/* (c) 1986 + Romstar */
	&trojanj_driver,	/* (c) 1986 */
	&srumbler_driver,	/* (c) 1986 */	/* aka Rush'n Crash */
	&srumblr2_driver,	/* (c) 1986 */
	&lwings_driver,		/* (c) 1986 */
	&lwings2_driver,	/* (c) 1986 */
	&lwingsjp_driver,	/* (c) 1986 */
	&sidearms_driver,	/* (c) 1986 */
	&sidearmr_driver,	/* (c) 1986 + Romstar license */
	&sidearjp_driver,	/* (c) 1986 */
	&avengers_driver,	/* (c) 1987 */
	&avenger2_driver,	/* (c) 1987 */
	&bionicc_driver,	/* (c) 1987 */	/* aka Top Secret */
	&bionicc2_driver,	/* (c) 1987 */
	&c1943_driver,		/* (c) 1987 */
	&c1943jap_driver,	/* (c) 1987 */
	&blktiger_driver,	/* (c) 1987 */
	&bktigerb_driver,	/* bootleg */
	&blkdrgon_driver,	/* (c) 1987 */
	/* Aug 1987: Street Fighter */
	&tigeroad_driver,	/* (c) 1987 + Romstar */
	&f1dream_driver,	/* (c) 1988 + Romstar */
	&f1dreamb_driver,	/* bootleg */
	&c1943kai_driver,	/* (c) 1987 */
	&lastduel_driver,	/* (c) 1988 */
	&lstduelb_driver,	/* (c) 1988 */
	/* Jul 1988: Forgotten Worlds (CPS1) */
	&ghouls_driver,		/* (c) 1988 (CPS1) */
	&ghoulsj_driver,	/* (c) 1988 (CPS1) */
	&madgear_driver,	/* (c) 1989 */	/* aka Led Storm */
	&strider_driver,	/* (c) 1989 (CPS1) */
	&striderj_driver,	/* (c) 1989 (CPS1) */
	/* Mar 1989: Dokaben */
	&dwj_driver,		/* (c) 1989 (CPS1) */
	&willow_driver,		/* (c) 1989 (CPS1) */
	&willowj_driver,	/* (c) 1989 (CPS1) */
	/* Aug 1989: Dokaben 2 */
	&unsquad_driver,	/* (c) 1989 (CPS1) */
	&area88_driver,		/* (c) 1989 (CPS1) */
	/* Oct 1989: Capcom Baseball */
	/* Nov 1989: Capcom World */
	&ffight_driver,		/* (c) [1989] (CPS1) */
	&ffightj_driver,	/* (c) [1989] (CPS1) */
	&c1941_driver,		/* (c) 1990 (CPS1) */
	&c1941j_driver,		/* (c) 1990 (CPS1) */
	/* Mar 1990: "capcomq" */
	&mercs_driver,		/* (c) 1990 (CPS1) */
	&mercsu_driver,		/* (c) 1990 (CPS1) */
	&mercsj_driver,		/* (c) 1990 (CPS1) */
	&mtwins_driver,		/* (c) 1990 (CPS1) */
	&chikij_driver,		/* (c) 1990 (CPS1) */
	&msword_driver,		/* (c) 1990 (CPS1) */
	&mswordu_driver,	/* (c) 1990 (CPS1) */
	&mswordj_driver,	/* (c) 1990 (CPS1) */
	&cawing_driver,		/* (c) 1990 (CPS1) */
	&cawingj_driver,	/* (c) 1990 (CPS1) */
	&nemo_driver,		/* (c) 1990 (CPS1) */
	&nemoj_driver,		/* (c) 1990 (CPS1) */
	/* Jan 1991: "tonosama" */
	&sf2_driver,		/* (c) 1991 (CPS1) */
	&sf2j_driver,		/* (c) 1991 (CPS1) */
	/* Apr 1991: "golf" */
	/* May 1991: Ataxx */
	/* Jun 1991: "sangoku" */
	/* Jul 1991: Three Wonders (CPS1) */
	&kod_driver,		/* (c) 1991 (CPS1) */
	&kodb_driver,		/* bootleg */
	/* Oct 1991: Block Block */
	&captcomm_driver,	/* (c) 1991 (CPS1) */
	&knights_driver,	/* (c) 1991 (CPS1) */
	/* Apr 1992: Street Fighter II' (CPS1) */
	&varth_driver,		/* (c) 1992 (CPS1) */
	/* Sep 1992: Capcom World 2 */
	/* Nov 1992: Warriors of Fate / Tenchi o Kurau 2 (CPS1) */
	/* Dec 1992: Street Fighter II Turbo (CPS1) */
	/* Apr 1993: Cadillacs & Dinosaurs (CPS1) */
	/* May 1993: Punisher (CPS1) */
	/* Jul 1993: Slam Masters (CPS1) */
	/* Oct 1993: Super Street Fighter II (CPS2) */
	/* Dec 1993: Muscle Bomber Duo (CPS1) */
	/* Jan 1994: Dungeons & Dragons - Tower of Doom (CPS2) */
	/* Mar 1994: Super Street Fighter II Turbo (CPS2) */
	/* May 1994: Alien vs Predator (CPS2) */
	/* Jun 1994: Eco Fighters (CPS2) */
	&pnickj_driver,		/* (c) 1994 + Compile license (CPS2?) not listed on Capcom's site? */
	/* another unlisted puzzle game: Gulum Pa! (CPS2) */
	/* Jul 1994: Dark Stalkers (CPS2) */
	/* Sep 1994: Ring of Destruction - Slam Masters 2 (CPS2) */
	/* Sep 1994: Quiz and Dragons (CPS1?) */
	/* Oct 1994: Armored Warriors (CPS2) */
	/* Dec 1994: X-Men - Children of the Atom */
	/* Jan 1995: "tonosama2" */
	/* Mar 1995: Night Warriors - Dark Stalkers Revenge (CPS2) */
	/* Apr 1995: Cyberbots (CPS2) */
	/* Jun 1995: Street Fighter - The Movie (CPS2) */
	/* Jun 1995: Street Fighter Alpha (CPS2) */
	&megaman_driver,	/* (c) 1995 (CPS1) */
	&rockmanj_driver,	/* (c) 1995 (CPS1) */
	/* Nov 1995: Marvel Super Heroes */
	/* Nov 1995: Battle Arena Toshinden 2 (3D, not CPS) */
	/* Jan 1996: 19XX - The War Against Destiny (CPS2) */
	/* Feb 1996: Dungeons & Dragons - Shadow Over Mystara (CPS2) */
	/* Mar 1996: Street Fighter Alpha 2 (CPS2) */
	/* Jun 1996: Super Puzzle Fighter II Turbo (CPS2) */
	/* Jul 1996: Star Gladiator (3D, not CPS) */
	/* Jul 1996: Megaman 2 - The Power Fighters (CPS2) */
	/* Aug 1996: Street Fighter Zero 2 Alpha (CPS2) */
	/* Sep 1996: "niji" */
	/* Sep 1996: X-Men vs. Street Fighter (CPS2) */
	/* Oct 1996: War-Zard */
	/* Dec 1996: Street Fighter EX */
	/* Feb 1997: Street Fighter III */
	/* Apr 1997: Street Fighter EX Plus */

	/* "Capcom Bowling hardware" games */
	&capbowl_driver,	/* (c) 1988 Incredible Technologies */
	&clbowl_driver,		/* (c) 1989 Incredible Technologies */
	&bowlrama_driver,	/* (c) 1991 P & P Marketing */
/*
The Incredible Technologies game list

Arlington Horse Racing
Blood Storm
Capcom Bowling
Coors Light Bowling
Driver's Edge
Dyno Bop
Golden Tee Golf '98
Golden Tee Golf '97
Golden Tee Golf II
Golden Tee Golf I
Hard Yardage Football
Hot Shots Tennis
Neck & Neck
Ninja Clowns
Pairs
Peggle
Poker Dice
Rim Rockin Basketball
Shuffleshot
Slick Shot
Strata Bowling
Street Fighter the Movie
Time Killers
Wheel of Fortune
World Class Bowling
*/

	/* Mitchell games */
	&pang_driver,		/* (c) 1990 Mitchell */
//	&bbros_driver,
//	&spang_driver,
//	&block_driver,

	/* Gremlin 8080 games */
	/* the numbers listed are the range of ROM part numbers */
	&blockade_driver,	/* 1-4 [1977 Gremlin] */
	&comotion_driver,	/* 5-7 [1977 Gremlin] */
	&hustle_driver,		/* 16-21 [1977 Gremlin] */
	&blasto_driver,		/* [1978 Gremlin] */

	/* Gremlin/Sega "VIC dual game board" games */
	/* the numbers listed are the range of ROM part numbers */
	&depthch_driver,	/* 50-55 [1977 Gremlin?] */
	&safari_driver,		/* 57-66 [1977 Gremlin?] */
	&frogs_driver,		/* 112-119 [1978 Gremlin?] */
	&sspaceat_driver,	/* 139-146 (c) */
	&headon_driver,		/* 163-167/192-193 (c) Gremlin */
	/* ???-??? Fortress */
	/* ???-??? Gee Bee */
	/* 255-270  Head On 2 / Deep Scan */
	&invho2_driver,		/* 271-286 (c) 1979 Sega */
	&samurai_driver,	/* 289-302 + upgrades (c) 1980 Sega */
	&invinco_driver,	/* 310-318 (c) 1979 Sega */
	&invds_driver,		/* 367-382 (c) 1979 Sega */
	&tranqgun_driver,	/* 413-428 (c) 1980 Sega */
	/* 450-465  Tranquilizer Gun (different version?) */
	/* ???-??? Car Hunt / Deep Scan */
	&spacetrk_driver,	/* 630-645 (c) 1980 Sega */
	&sptrekct_driver,	/* (c) 1980 Sega */
	&carnival_driver,	/* 651-666 (c) 1980 Sega */
	&pulsar_driver,		/* 790-805 (c) 1981 Sega */
	&heiankyo_driver,	/* (c) [1979?] Denki Onkyo */

	/* Sega G-80 vector games */
	&spacfury_driver,	/* (c) 1981 */
	&spacfura_driver,	/* no copyright notice */
	&zektor_driver,		/* (c) 1982 */
	&tacscan_driver,	/* (c) */
	&elim2_driver,		/* (c) 1981 Gremlin */
	&elim4_driver,		/* (c) 1981 Gremlin */
	&startrek_driver,	/* (c) 1982 */

	/* Sega G-80 raster games */
	&astrob_driver,		/* (c) 1981 */
	&astrob1_driver,	/* (c) 1981 */
	&s005_driver,		/* (c) 1981 */
	&monsterb_driver,	/* (c) 1982 */
	&spaceod_driver,	/* (c) 1981 */

	/* Sega "Zaxxon hardware" games */
	&zaxxon_driver,		/* (c) 1982 */
	&szaxxon_driver,	/* (c) 1982 */
	&futspy_driver,		/* (c) 1984 */
	&congo_driver,		/* 605-5167 (c) 1983 */
	&tiptop_driver,		/* 605-5167 (c) 1983 */

	/* Sega System 8 games */
	&starjack_driver,	/* 834-5191 (c) 1983 */
	&starjacs_driver,	/* (c) 1983 Stern */
	&regulus_driver,	/* 834-5328�(c) 1983 */
	&regulusu_driver,	/* 834-5328�(c) 1983 */
	&upndown_driver,	/* (c) 1983 */
	&mrviking_driver,	/* 834-5383 (c) 1984 */
	&swat_driver,		/* 834-5388 (c) 1984 Coreland / Sega */
	&flicky_driver,		/* (c) 1984 */
	&flicky2_driver,	/* (c) 1984 */
	/* Water Match */
	&bullfgtj_driver,	/* 834-5478 (c) 1984 Sega / Coreland */
	&pitfall2_driver,	/* 834-5627 [1985?] reprogrammed, (c) 1984 Activision */
	&pitfallu_driver,	/* 834-5627 [1985?] reprogrammed, (c) 1984 Activision */
	&seganinj_driver,	/* 834-5677 (c) 1985 */
	&seganinu_driver,	/* 834-5677 (c) 1985 */
	&nprinces_driver,	/* 834-5677 (c) 1985 */
	&nprinceb_driver,	/* 834-5677 (c) 1985 */
	&imsorry_driver,	/* 834-5707 (c) 1985 Coreland / Sega */
	&imsorryj_driver,	/* 834-5707 (c) 1985 Coreland / Sega */
	&teddybb_driver,	/* 834-5712 (c) 1985 */
	&hvymetal_driver,	/* 834-5745 (c) 1985 */
	&myhero_driver,		/* 834-5755 (c) 1985 */
	&myheroj_driver,	/* 834-5755 (c) 1985 Coreland / Sega */
	&chplft_driver,		/* 834-5795 (c) 1985, (c) 1982 Dan Gorlin */
	&chplftb_driver,	/* 834-5795 (c) 1985, (c) 1982 Dan Gorlin */
	&chplftbl_driver,	/* bootleg */
	&fdwarrio_driver,	/* 834-5918 (c) 1985 Coreland / Sega */
	&brain_driver,		/* (c) 1986 Coreland / Sega */
	&wboy_driver,		/* 834-5984 (c) 1986 + Escape license */
	&wboy2_driver,		/* 834-5984 (c) 1986 + Escape license */
	&wboy3_driver,
	&wboy4_driver,		/* 834-5984 (c) 1986 + Escape license */
	&wboyu_driver,		/* 834-5753 (? maybe a conversion) (c) 1986 + Escape license */
	&wboy4u_driver,		/* 834-5984 (c) 1986 + Escape license */
	&wbdeluxe_driver,	/* (c) 1986 + Escape license */
	&gardia_driver,		/* 834-6119 */
	&blockgal_driver,	/* 834-6303 */
	&tokisens_driver,	/* (c) 1987 (from a bootleg board) */
	&dakkochn_driver,	/* 836-6483? */
	&ufosensi_driver,	/* 834-6659 */
	&wbml_driver,		/* bootleg */
/*
other System 8 games:

WarBall
Rafflesia
Sanrin Sanchan
DokiDoki Penguin Land *not confirmed
*/

	/* Sega System 16 games */
	&alexkidd_driver,	/* (c) 1986 (but bootleg) */
	&aliensyn_driver,	/* (c) 1987 */
	&altbeast_driver,	/* (c) 1988 */
	&astormbl_driver,
	&aurail_driver,
	&dduxbl_driver,
	&eswatbl_driver,	/* (c) 1989 (but bootleg) */
	&fantzone_driver,
	&fpointbl_driver,
	&goldnaxe_driver,	/* (c) 1989 */
	&hwchamp_driver,
	&mjleague_driver,	/* (c) 1985 */
	&passshtb_driver,	/* bootleg */
	&quartet2_driver,
	&sdi_driver,		/* (c) 1987 */
	&shinobi_driver,	/* (c) 1987 */
	&tetrisbl_driver,	/* (c) 1988 (but bootleg) */
	&timscanr_driver,
	&tturfbl_driver,
	&wb3bl_driver,
	&wrestwar_driver,	/* (c) 1989 */

	/* Data East "Burger Time hardware" games */
	&lnc_driver,		/* (c) 1981 */
	&zoar_driver,		/* (c) 1982 */
	&btime_driver,		/* (c) 1982 + Midway */
	&btimea_driver,		/* (c) 1982 */
	&cookrace_driver,	/* bootleg */
	&bnj_driver,		/* (c) 1982 + Midway */
	&brubber_driver,	/* (c) 1982 */
	&caractn_driver,	/* bootleg */
	&eggs_driver,		/* (c) 1983 Universal USA */
	&scregg_driver,		/* TA-0001 (c) 1983 Technos Japan */
	&tagteam_driver,	/* TA-0007 (c) 1983 + Technos Japan license */

	/* other Data East games */
	&astrof_driver,		/* (c) [1980?] */
	&astrof2_driver,	/* (c) [1980?] */
	&astrof3_driver,	/* (c) [1980?] */
	&tomahawk_driver,	/* (c) [1980?] */
	&tomahaw5_driver,	/* (c) [1980?] */
	&kchamp_driver,		/* (c) 1984 Data East USA */
	&kchampvs_driver,	/* (c) 1984 Data East USA */
	&karatedo_driver,	/* (c) 1984 Data East Corporation */
	&firetrap_driver,	/* (c) 1986 */
	&firetpbl_driver,	/* bootleg */
	&brkthru_driver,	/* (c) 1986 */
	&darwin_driver,
	&shootout_driver,	/* (c) 1985 */
	&sidepckt_driver,	/* (c) 1986 */
	&exprraid_driver,	/* (c) 1986 Data East USA */
	&wexpress_driver,	/* (c) 1986 Data East Corporation */
	&wexpresb_driver,	/* bootleg */

	/* Data East 8-bit games */
	&ghostb_driver,		/* (c) 1987 Data East USA */
	&mazeh_driver,		/* (c) 1987 Data East Corporation */
//	&srdarwin_driver,
	&cobracom_driver,	/* (c) 1988 Data East Corporation */
//	&gondo_driver,
	&oscar_driver,		/* (c) 1988 Data East USA */
	&oscarj_driver,		/* (c) 1988 Data East Corporation */
//	&lastmiss_driver,
//	&shackled_driver,

	/* Data East 16-bit games */
	&karnov_driver,		/* (c) 1987 Data East USA */
	&karnovj_driver,	/* (c) 1987 Data East Corporation */
	&chelnov_driver,	/* (c) 1988 Data East USA */
	&chelnovj_driver,	/* (c) 1988 Data East Corporation */
	/* the following ones all run on similar hardware */
	&hbarrel_driver,	/* (c) 1987 Data East USA */
	&hbarrelj_driver,	/* (c) 1987 Data East Corporation */
	&baddudes_driver,	/* (c) 1988 Data East USA */
	&drgninja_driver,	/* (c) 1988 Data East Corporation */
	&robocopp_driver,	/* (c) 1988 Data East Corp. */
	&hippodrm_driver,	/* (c) 1989 Data East USA */
	&ffantasy_driver,	/* (c) 1989 Data East Corporation */
	&slyspy_driver,		/* (c) 1989 Data East USA */
	&midres_driver,		/* (c) 1989 Data East USA */
	&midresj_driver,	/* (c) 1989 Data East Corporation */
	/* evolution of the hardware */
	&darkseal_driver,	/* (c) 1990 Data East USA */
	&gatedoom_driver,	/* (c) 1990 Data East Corporation */

	/* Tehkan / Tecmo games (Tehkan became Tecmo in 1986) */
	&bombjack_driver,	/* (c) 1984 Tehkan */
	&starforc_driver,	/* (c) 1984 Tehkan */
	&megaforc_driver,	/* (c) 1985 Tehkan + Video Ware license */
	&pbaction_driver,	/* (c) 1985 Tehkan */
	&pbactio2_driver,	/* (c) 1985 Tehkan */
	&tehkanwc_driver,	/* (c) 1985 Tehkan */
	&gridiron_driver,	/* (c) 1985 Tehkan */
	&teedoff_driver,	/* 6102 - (c) 1986 Tecmo */
	&solomon_driver,	/* (c) 1986 Tecmo */
	&rygar_driver,		/* 6002 - (c) 1986 Tecmo */
	&rygar2_driver,		/* 6002 - (c) 1986 Tecmo */
	&rygarj_driver,		/* 6002 - (c) 1986 Tecmo */
	&gemini_driver,		/* (c) 1987 Tecmo */
	&silkworm_driver,	/* 6217 - (c) 1988 Tecmo */
	&silkwrm2_driver,	/* 6217 - (c) 1988 Tecmo */
	&gaiden_driver,		/* 6215 - (c) 1988 Tecmo */
	&shadoww_driver,	/* 6215 - (c) 1988 Tecmo */
	&tknight_driver,	/* (c) 1989 Tecmo */
	&wc90_driver,		/* (c) 1989 Tecmo */
	&wc90b_driver,		/* bootleg */

	/* Konami games */
	&pooyan_driver,		/* GX320 (c) 1982 */
	&pooyans_driver,	/* GX320 (c) 1982 Stern */
	&pootan_driver,		/* bootleg */
	&timeplt_driver,	/* GX393 (c) 1982 */
	&timepltc_driver,	/* GX393 (c) 1982 + Centuri license*/
	&spaceplt_driver,	/* bootleg */
	&rocnrope_driver,	/* GX364 (c) 1983 + Kosuka */
	&ropeman_driver,	/* bootleg */
	&venus_driver,		/* bootleg */
	&trackfld_driver,	/* GX361 (c) 1983 */
	&trackflc_driver,	/* GX361 (c) 1983 + Centuri license */
	&hyprolym_driver,	/* GX361 (c) 1983 */
	&hyprolyb_driver,	/* bootleg */
	&circusc_driver,	/* GX380 (c) 1984 */
	&circusc2_driver,	/* GX380 (c) 1984 */
	&tp84_driver,		/* GX388 (c) 1984 */
	&tp84a_driver,		/* GX388 (c) 1984 */
	&hyperspt_driver,	/* GX330 (c) 1984 + Centuri */
	&sbasketb_driver,	/* GX405 (c) 1984 */
	&mikie_driver,		/* GX469 (c) 1984 */
	&mikiej_driver,		/* GX469 (c) 1984 */
	&mikiehs_driver,	/* GX469 (c) 1984 */
	&roadf_driver,		/* GX461 (c) 1984 */
	&roadf2_driver,		/* GX461 (c) 1984 */
	&yiear_driver,		/* GX407 (c) 1985 */
	&kicker_driver,		/* GX477 (c) 1985 */
	&shaolins_driver,	/* GX477 (c) 1985 */
	&pingpong_driver,	/* GX555 (c) 1985 */
	&gberet_driver,		/* GX577 (c) 1985 */
	&rushatck_driver,	/* GX577 (c) 1985 */
	&jailbrek_driver,	/* GX507 (c) 1986 */
	&ironhors_driver,	/* GX560 (c) 1986 */
	&farwest_driver,
	&jackal_driver,		/* GX631 (c) 1986 */
	&topgunr_driver,	/* GX631 (c) 1986 */
	&topgunbl_driver,	/* bootleg */
	&mainevt_driver,	/* GX799 (c) 1988 */
	&devstors_driver,	/* GX890 (c) 1988 */

	/* Konami "Nemesis hardware" games */
	&nemesis_driver,	/* GX456 (c) 1985 */
	&nemesuk_driver,	/* GX456 (c) 1985 */
	&konamigt_driver,	/* GX561 (c) 1985 */
//	&salamand_driver,
	/* GX400 BIOS based games */
	&rf2_driver,		/* GX561 (c) 1985 */
	&twinbee_driver,	/* GX412 (c) 1985 */
	&gradius_driver,	/* GX456 (c) 1985 */
	&gwarrior_driver,	/* GX578 (c) 1985 */

	/* Konami "TMNT hardware" games */
	&tmnt_driver,		/* GX963 (c) 1989 */
	&tmntj_driver,		/* GX963 (c) 1989 */
	&tmht2p_driver,		/* GX963 (c) 1989 */
	&tmnt2pj_driver,	/* GX963 (c) 1990 */
	&punkshot_driver,	/* GX907 (c) 1990 */

	/* Exidy games */
	&sidetrac_driver,	/* (c) 1979 */
	&targ_driver,		/* (c) 1980 */
	&spectar_driver,	/* (c) 1980 */
	&spectar1_driver,	/* (c) 1980 */
	&venture_driver,	/* (c) 1981 */
	&venture2_driver,	/* (c) 1981 */
	&venture4_driver,	/* (c) 1981 */
	&mtrap_driver,		/* (c) 1981 */
	&pepper2_driver,	/* (c) 1982 */
	&hardhat_driver,	/* (c) 1982 */
	&fax_driver,		/* (c) 1983 */
	&circus_driver,		/* no copyright notice [1977?] */
	&robotbwl_driver,	/* no copyright notice */
	&crash_driver,		/* Exidy [1979?] */
	&ripcord_driver,	/* Exidy [1977?] */
	&starfire_driver,	/* Exidy */

	/* Atari vector games */
	&asteroid_driver,	/* (c) 1979 */
	&asteroi1_driver,	/* no copyright notice */
	&astdelux_driver,	/* (c) 1980 */
//	&astdelu1_driver,
	&bwidow_driver,		/* (c) 1982 */
	&bzone_driver,		/* (c) 1980 */
	&bzone2_driver,		/* (c) 1980 */
	&gravitar_driver,	/* (c) 1982 */
	&llander_driver,	/* no copyright notice */
	&llander1_driver,	/* no copyright notice */
	&redbaron_driver,	/* (c) 1980 */
	&spacduel_driver,	/* (c) 1980 */
	&tempest_driver,	/* (c) 1980 */
	&tempest1_driver,	/* (c) 1980 */
	&tempest2_driver,	/* (c) 1980 */
	&temptube_driver,	/* hack */
	&starwars_driver,	/* (c) 1983 */
	&empire_driver,		/* (c) 1985 */
	&mhavoc_driver,		/* (c) 1983 */
	&mhavoc2_driver,	/* (c) 1983 */
	&mhavocrv_driver,	/* hack */
	&quantum_driver,	/* (c) 1982 */
	&quantum1_driver,	/* (c) 1982 */

	/* Atari "Centipede hardware" games */
	&warlord_driver,	/* (c) 1980 */
	&centiped_driver,	/* (c) 1980 */
	&centipd2_driver,	/* (c) 1980 */
	&milliped_driver,	/* (c) 1982 */
	&qwakprot_driver,	/* (c) 1982 */

	/* Atari "Kangaroo hardware" games */
	&kangaroo_driver,	/* (c) 1982 */
	&kangarob_driver,	/* bootleg */
	&arabian_driver,	/* (c) 1983 */

	/* Atari "Missile Command hardware" games */
	&missile_driver,	/* (c) 1980 */
	&missile2_driver,	/* (c) 1980 */
	&suprmatk_driver,	/* (c) 1980 + (c) 1981 Gencomp */

	/* Atari b/w games */
	&sprint1_driver,	/* no copyright notice */
	&sprint2_driver,	/* no copyright notice */
	&sbrkout_driver,	/* no copyright notice */
	&dominos_driver,	/* no copyright notice */
	&nitedrvr_driver,	/* no copyright notice [1976] */
	&bsktball_driver,	/* no copyright notice */
	&copsnrob_driver,	/* [1976] */
	&avalnche_driver,	/* no copyright notice [1978] */
	&subs_driver,		/* no copyright notice [1976] */

	/* Atari System 1 games */
	&marble_driver,		/* (c) 1984 */
	&marble2_driver,	/* (c) 1984 */
	&marblea_driver,	/* (c) 1984 */
	&peterpak_driver,	/* (c) 1984 */
	&indytemp_driver,	/* (c) 1985 */
	&roadrunn_driver,	/* (c) 1985 */
	&roadblst_driver,	/* (c) 1986, 1987 */

	/* Atari System 2 games */
	&paperboy_driver,	/* (c) 1984 */
	&ssprint_driver,	/* (c) 1986 */
	&csprint_driver,	/* (c) 1986 */
	&a720_driver,		/* (c) 1986 */
	&a720b_driver,		/* (c) 1986 */
	&apb_driver,		/* (c) 1987 */
	&apb2_driver,		/* (c) 1987 */

	/* later Atari games */
	&gauntlet_driver,	/* (c) 1985 */
	&gauntir1_driver,	/* (c) 1985 */
	&gauntir2_driver,	/* (c) 1985 */
	&gaunt2p_driver,	/* (c) 1985 */
	&gaunt2_driver,		/* (c) 1986 */
	&atetris_driver,	/* (c) 1988 */
	&atetrisa_driver,	/* (c) 1988 */
	&atetrisb_driver,	/* bootleg */
	&atetcktl_driver,	/* (c) 1989 */
	&atetckt2_driver,	/* (c) 1989 */
	&toobin_driver,		/* (c) 1988 */
	&vindictr_driver,	/* (c) 1988 */
	&klax_driver,		/* (c) 1989 */
	&klax2_driver,		/* (c) 1989 */
	&klax3_driver,		/* (c) 1989 */
	&blstroid_driver,	/* (c) 1987 */
	&eprom_driver,		/* (c) 1989 */
	&xybots_driver,		/* (c) 1987 */

	/* SNK / Rock-ola games */
	&sasuke_driver,		/* [1980] Shin Nihon Kikaku (SNK) */
	&satansat_driver,	/* (c) 1981 SNK */
	&zarzon_driver,		/* (c) 1981 Taito, gameplay says SNK */
	&vanguard_driver,	/* (c) 1981 SNK */
	&vangrdce_driver,	/* (c) 1981 SNK + Centuri */
	&fantasy_driver,	/* (c) 1981 Rock-ola */
	&pballoon_driver,	/* (c) 1982 SNK */
	&nibbler_driver,	/* (c) 1982 Rock-ola */
	&nibblera_driver,	/* (c) 1982 Rock-ola */

	/* Technos games */
	&mystston_driver,	/* (c) 1984 */
	&matmania_driver,	/* TA-0015 (c) 1985 + Taito America license */
	&excthour_driver,	/* TA-0015 (c) 1985 + Taito license */
	&maniach_driver,	/* TA-???? (c) 1986 + Taito America license */
	&maniach2_driver,	/* TA-???? (c) 1986 + Taito America license */
	&renegade_driver,
	&kuniokub_driver,	/* TA-0018 bootleg */
	&xsleena_driver,	/* TA-0019 (c) 1986 */
	&solarwar_driver,	/* TA-0019 (c) 1986 + Memetron license */
	&ddragon_driver,
	&ddragonb_driver,	/* TA-0021 bootleg */
	/* TA-0023 China Gate */
	/* TA-0024 WWF Superstars */
	/* TA-0025 Champ V'Ball */
	&ddragon2_driver,	/* TA-0026 (c) 1988 */
	/* TA-0028 Combatribes */
	&blockout_driver,	/* TA-0029 (c) 1989 + California Dreams */
	/* TA-0030 Double Dragon 3 */
	/* TA-0031 WWF Wrestlefest */

	/* Stern "Berzerk hardware" games */
	&berzerk_driver,	/* (c) 1980 */
	&berzerk1_driver,	/* (c) 1980 */
	&frenzy_driver,		/* (c) 1982 */
	&frenzy1_driver,	/* (c) 1982 */

	/* GamePlan games */
	&megatack_driver,	/* (c) 1980 Centuri */
	&killcom_driver,	/* (c) 1980 Centuri */
	&challeng_driver,	/* (c) 1981 Centuri */
	&kaos_driver,		/* (c) 1981 */

	/* "stratovox hardware" games */
	&route16_driver,	/* (c) 1981 Leisure and Allied (bootleg) */
	&stratvox_driver,	/* Taito */
	&speakres_driver,	/* no copyright notice */

	/* Zaccaria games */
	&monymony_driver,	/* (c) 1983 */
	&jackrabt_driver,	/* (c) 1984 */
	&jackrab2_driver,	/* (c) 1984 */
	&jackrabs_driver,	/* (c) 1984 */

	/* UPL games */
	&nova2001_driver,	/* (c) [1984?] + Universal license */
	&pkunwar_driver,	/* [1985?] */
	&pkunwarj_driver,	/* [1985?] */
	&ninjakd2_driver,	/* (c) 1987 */
	&ninjak2a_driver,	/* (c) 1987 */
/*
other UPL games:

Ark Area
Atomic Robo Kid
Bio Ship Paladin/Spaceship Gomera
Omega Fighter (Shooter)
Black Heart� (Shooter)
Mission XX (Shooter)
Rad Action
*/

	/* Williams/Midway TMS34010 games */
	&narc_driver,		/* (c) 1988 Williams */
	&trog_driver,		/* (c) 1990 Midway */
	&trog3_driver,		/* (c) 1990 Midway */
	&trogp_driver,		/* (c) 1990 Midway */
	&smashtv_driver,	/* (c) 1990 Williams */
	&smashtv5_driver,	/* (c) 1990 Williams */
	&hiimpact_driver,	/* (c) 1990 Williams */
	&shimpact_driver,	/* (c) 1991 Midway */
	&strkforc_driver,	/* (c) 1991 Midway */
	&mk_driver,			/* (c) 1992 Midway */
	&term2_driver,		/* (c) 1992 Midway */
	&totcarn_driver,	/* (c) 1992 Midway */
	&totcarnp_driver,	/* (c) 1992 Midway */
	&mk2_driver,		/* (c) 1993 Midway */
	&nbajam_driver,		/* (c) 1993 Midway */

	/* Cinematronics raster games */
	&jack_driver,		/* (c) 1982 Cinematronics */
	&jacka_driver,		/* (c) 1982 Cinematronics */
	&treahunt_driver,
	&zzyzzyxx_driver,	/* (c) 1982 Cinematronics */
	&brix_driver,		/* (c) 1982 Cinematronics */
	&sucasino_driver,	/* (c) 1982 Data Amusement */

	/* "The Pit hardware" games */
	&roundup_driver,	/* (c) 1981 Amenip/Centuri */
	&thepit_driver,		/* (c) 1982 Centuri */
	&intrepid_driver,	/* (c) 1983 Nova Games Ltd. */
	&portman_driver,	/* (c) 1982 Nova Games Ltd. */
	&suprmous_driver,	/* (c) 1982 Taito */

	/* Valadon Automation games */
	&bagman_driver,		/* (c) 1982 */
	&bagmans_driver,	/* (c) 1982 + Stern license */
	&bagmans2_driver,	/* (c) 1982 + Stern license */
	&sbagman_driver,	/* (c) 1984 */
	&sbagmans_driver,	/* (c) 1984 + Stern license */
	&pickin_driver,		/* (c) 1983 */

	/* Seibu Denshi / Seibu Kaihatsu games */
	&stinger_driver,	/* (c) 1983 Seibu Denshi */
	&scion_driver,		/* (c) 1984 Seibu Denshi */
	&scionc_driver,		/* (c) 1984 Seibu Denshi + Cinematronics license */
	&wiz_driver,		/* (c) 1985 Seibu Kaihatsu */

	/* Nichibutsu games */
	&cop01_driver,		/* (c) 1985 */
	&cop01a_driver,		/* (c) 1985 */
	&terracre_driver,	/* (c) 1985 */
	&terracra_driver,	/* (c) 1985 */
	&galivan_driver,	/* (c) 1985 */

	/* Neogeo MGD2 rom dumps */
	&joyjoy_driver,     /* 1990, SNK */
	&ridhero_driver,    /* 1990, SNK */
	&bstars_driver,     /* 1990, SNK */
	&lbowling_driver,   /* 1990, SNK */
	&superspy_driver,   /* 1990, SNK */
	&ttbb_driver,       /* 1991, SNK */
	&alpham2_driver,    /* 1991, SNK */
	&eightman_driver,   /* 1991, SNK */
	&ararmy_driver,     /* 1991, SNK */
	&fatfury1_driver,   /* 1991, SNK */
	&burningf_driver,   /* 1991, SNK */
	&kingofm_driver,    /* 1991, SNK */
	&gpilots_driver,    /* 1991, SNK */
	&lresort_driver,    /* 1992, SNK */
	&fbfrenzy_driver,   /* 1992, SNK */
	&socbrawl_driver,   /* 1992, SNK */
	&mutnat_driver,     /* 1992, SNK */
	&artfight_driver,   /* 1992, SNK */
	&countb_driver,     /* 1993, SNK */
	&ncombat_driver,    /* 1990, Alpha Denshi Co */
	&crsword_driver,    /* 1991, Alpha Denshi Co */
	&trally_driver,     /* 1991, Alpha Denshi Co */
	&sengoku_driver,    /* 1991, Alpha Denshi Co */
	&ncommand_driver,   /* 1992, Alpha Denshi Co */
	&wheroes_driver,    /* 1992, Alpha Denshi Co */
	&sengoku2_driver,   /* 1993, Alpha Denshi Co */
	&androdun_driver,   /* 1992, Visco */

	/* Neogeo MVS rom dumps */
	&bjourney_driver,   /* 1990, Alpha Denshi Co */
	&maglord_driver,    /* 1990, Alpha Denshi Co */
	&janshin_driver,    /* 1994, Aicom */
	&pulstar_driver,    /* 1995, Aicom */
	&blazstar_driver,   /* 1998, Yumekobo */
	&pbobble_driver,    /* 1994, Taito */
	&puzzledp_driver,   /* 1995, Taito (Visco license) */
	&pspikes2_driver,   /* 1994, Visco */
	&sonicwi2_driver,   /* 1994, Visco */
	&sonicwi3_driver,   /* 1995, Visco */
	&goalx3_driver,     /* 1995, Visco */
	&neodrift_driver,   /* 1996, Visco */
	&neomrdo_driver,    /* 1996, Visco */
	&spinmast_driver,   /* 1993, Data East Corporation */
	&karnov_r_driver,   /* 1994, Data East Corporation */
	&wjammers_driver,   /* 1994, Data East Corporation */
	&strhoops_driver,   /* 1994, Data East Corporation */
	&magdrop2_driver,   /* 1996, Data East Corporation */
	&magdrop3_driver,   /* 1997, Data East Corporation */
	&mslug_driver,      /* 1996, Nazca */
	&turfmast_driver,   /* 1996, Nazca */
	&kabukikl_driver,   /* 1995, Hudson */
	&panicbom_driver,   /* 1995, Hudson */
	&neobombe_driver,   /* 1997, Hudson */
	&worlher2_driver,   /* 1993, ADK */
	&worlhe2j_driver,   /* 1994, ADK */
	&aodk_driver,       /* 1994, ADK */
	&whp_driver,        /* 1995, ADK/SNK */
	&ninjamas_driver,   /* 1996, ADK/SNK */
	&overtop_driver,    /* 1996, ADK */
	&twinspri_driver,   /* 1997, ADK */
	&stakwin_driver,    /* 1995, Saurus */
	&ragnagrd_driver,   /* 1996, Saurus */
	&shocktro_driver,   /* 1997, Saurus */
	&tws96_driver,      /* 1996, Tecmo */
	&zedblade_driver,   /* 1994, NMK */
	&doubledr_driver,   /* 1995, Technos */
	&gowcaizr_driver,   /* 1995, Technos */
	&galaxyfg_driver,   /* 1995, Sunsoft */
	&wakuwak7_driver,   /* 1996, Sunsoft */
	&viewpoin_driver,   /* 1992, Sammy */
	&gururin_driver,    /* 1994, Face */
	&miexchng_driver,   /* 1997, Face */
	&mahretsu_driver,   /* 1990, SNK */
	&nam_1975_driver,   /* 1990, SNK */
	&cyberlip_driver,   /* 1990, SNK */
	&tpgolf_driver,     /* 1990, SNK */
	&legendos_driver,   /* 1991, SNK */
	&fatfury2_driver,   /* 1992, SNK */
	&bstars2_driver,    /* 1992, SNK */
	&sidkicks_driver,   /* 1992, SNK */
	&kotm2_driver,      /* 1992, SNK */
	&samsho_driver,     /* 1992, SNK */
	&fatfursp_driver,   /* 1993, SNK */
	&fatfury3_driver,   /* 1993, SNK */
	&tophuntr_driver,   /* 1994, SNK */
	&savagere_driver,   /* 1995, SNK */
	&kof94_driver,      /* 1994, SNK */
	&aof2_driver,       /* 1994, SNK */
	&ssideki2_driver,   /* 1994, SNK */
	&samsho2_driver,    /* 1994, SNK */
	&sskick3_driver,    /* 1995, SNK */
	&samsho3_driver,    /* 1995, SNK */
	&kof95_driver,      /* 1995, SNK */
	&rbff1_driver,      /* 1995, SNK */
	&aof3_driver,       /* 1996, SNK */
	&kof96_driver,      /* 1996, SNK */
	&samsho4_driver,    /* 1996, SNK */
	&rbffspec_driver,   /* 1996, SNK */
	&kizuna_driver,     /* 1996, SNK */
	&kof97_driver,      /* 1997, SNK */
	&lastblad_driver,   /* 1997, SNK */
	&mslug2_driver,     /* 1998, SNK */
	&realbou2_driver,   /* 1998, SNK */

	&spacefb_driver,	/* 834-0031 (c) [1980?] Nintendo */
	&tutankhm_driver,	/* GX350 (c) 1982 Konami */
	&tutankst_driver,	/* GX350 (c) 1982 Stern */
	&junofrst_driver,	/* GX310 (c) 1983 Konami */
	&ccastles_driver,	/* (c) 1983 Atari */
	&ccastle2_driver,	/* (c) 1983 Atari */
	&blueprnt_driver,	/* (c) 1982 Bally Midway */
	&omegrace_driver,	/* (c) 1981 Midway */
	&bankp_driver,		/* (c) 1984 Sega */
	&espial_driver,		/* (c) 1983 Thunderbolt */
	&espiale_driver,	/* (c) 1983 Thunderbolt */
	&cloak_driver,		/* (c) 1983 Atari */
	&champbas_driver,	/* (c) 1983 Sega */
	&champbb2_driver,
//	&sinbadm_driver,	/* 834-5244 */
	&exerion_driver,	/* (c) 1983 Jaleco */
	&exeriont_driver,	/* (c) 1983 Jaleco + Taito America license */
	&exerionb_driver,	/* bootleg */
	&foodf_driver,		/* (c) 1982 Atari */
	&vastar_driver,		/* (c) 1983 Sesame Japan */
	&vastar2_driver,	/* (c) 1983 Sesame Japan */
	&formatz_driver,	/* (c) 1984 Jaleco */
	&aeroboto_driver,	/* (c) 1984 Williams */
	&citycon_driver,	/* (c) 1985 Jaleco */
	&psychic5_driver,	/* (c) 1987 Jaleco */
	&jedi_driver,		/* (c) 1984 Atari */
	&tankbatt_driver,	/* (c) 1980 Namco */
	&liberatr_driver,	/* (c) 1982 Atari */
	&dday_driver,		/* (c) 1982 Centuri */
	&toki_driver,		/* (c) 1990 Datsu (bootleg) */
	&snowbros_driver,	/* (c) 1990 Toaplan + Romstar license */
	&snowbro2_driver,	/* (c) 1990 Toaplan + Romstar license */
	&gundealr_driver,	/* (c) 1990 Dooyong */
	&gundeala_driver,	/* (c) Dooyong */
	&leprechn_driver,	/* (c) 1982 Tong Electronic */
	&potogold_driver,	/* (c) 1982 Tong Electronic */
	&hexa_driver,		/* D. R. Korea */
	&redalert_driver,	/* (c) 1981 Irem (GDI game) */
	&irobot_driver,
	&spiders_driver,	/* (c) 1981 Sigma */
	&stactics_driver,	/* [1981 Sega] */
	&goldstar_driver,
	&goldstbl_driver,
	&exterm_driver,		/* (c) 1989 Premier Technology - a Gottlieb game */
	&sharkatt_driver,	/* (c) Pacific Novelty */
	&turbo_driver,		/* (c) 1981 Sega */
	&turboa_driver,		/* (c) 1981 Sega */
	&turbob_driver,		/* (c) 1981 Sega */
	&kingofb_driver,	/* (c) 1985 Woodplace Inc. */
	&kingofbj_driver,	/* (c) 1985 Woodplace Inc. */
	&ringking_driver,	/* (c) 1985 Data East USA */
	&ringkin2_driver,	/* (c) 1985 Data East USA */
	&zerozone_driver,	/* (c) 1993 Comad */
	&exctsccr_driver,	/* (c) 1983 Alpha Denshi Co. */
	&exctsccb_driver,	/* bootleg */
	&speedbal_driver,	/* (c) 1987 Tecfri */
	&sauro_driver,		/* (c) 1987 Tecfri */
	&pow_driver,		/* (c) 1988 SNK */
	&powj_driver,		/* (c) 1988 SNK */
#endif	
	0	/* end of array */
};
