// List of all engine variables and their types.
// Used to generate fo::ptr::ptr_name pointers.

PTR_(aiInfoList,                 DWORD)
PTR_(ambient_light,              DWORD)
PTR_(art,                        fo::Art) // array of 11 Art
PTR_(art_name,                   DWORD)
PTR_(art_vault_guy_num,          DWORD)
PTR_(art_vault_person_nums,      DWORD)
PTR_(background_volume,          DWORD)
PTR_(bckgnd,                     BYTE*)
PTR_(black_palette,              DWORD)
PTR_(BlueColor,                  BYTE)
PTR_(bottom_line,                DWORD)
PTR_(btable,                     DWORD)
PTR_(btncnt,                     DWORD)
PTR_(cap,                        fo::AIcap*) // dynamic array
PTR_(carCurrentArea,             DWORD)
PTR_(carGasAmount,               long) // from 0 to 80000
PTR_(cmap,                       fo::PALETTE) // array of 256 PALETTE
PTR_(colorTable,                 DWORD)
PTR_(combat_free_move,           DWORD)
PTR_(combat_list,                fo::GameObject**) // dynamic array
PTR_(combat_obj,                 fo::GameObject*)
PTR_(combat_state,               DWORD)
PTR_(combat_turn_obj,            fo::GameObject*)
PTR_(combat_turn_running,        DWORD)
PTR_(combatNumTurns,             DWORD)
PTR_(crit_succ_eff,              fo::CritInfo)  // array: 20 critters with 9 body parts and 6 effects each
PTR_(critter_db_handle,          fo::PathNode*)
PTR_(critterClearObj,            DWORD)
PTR_(crnt_func,                  DWORD)
PTR_(cur_id,                     DWORD)
PTR_(curr_anim_set,              DWORD)
PTR_(curr_anim_counter,          DWORD)
PTR_(curr_font_num,              DWORD)
PTR_(curr_pc_stat,               long) // array of size == PCSTAT_max_pc_stat
PTR_(curr_stack,                 DWORD)
PTR_(currentProgram,             fo::Program*)
PTR_(cursor_line,                DWORD)
PTR_(DarkGreenColor,             BYTE)
PTR_(DarkGreenGreyColor,         BYTE)
PTR_(DarkGreyColor,              BYTE)
PTR_(DarkRedColor,               BYTE)
PTR_(DarkYellowColor,            BYTE)
PTR_(dialog_target,              fo::GameObject*)
PTR_(dialog_target_is_party,     DWORD)
PTRC(dialogueBackWindow,         DWORD)
PTR_(drugInfoList,               fo::DrugInfoList) // array of 9 DrugInfoList
PTR_(DullPinkColor,              BYTE)
PTRC(edit_win,                   DWORD)
PTR_(Educated,                   DWORD)
PTR_(elevation,                  DWORD)
PTR_(endgame_subtitle_done,      DWORD)
PTR_(Experience_pc,              DWORD)
PTR_(fadeEventHandle,            DWORD)
PTR_(fallout_game_time,          DWORD)
PTR_(fidgetFID,                  DWORD)
PTR_(flptr,                      DWORD)
PTR_(folder_card_desc,           DWORD)
PTR_(folder_card_fid,            DWORD)
PTR_(folder_card_title,          DWORD)
PTR_(folder_card_title2,         DWORD)
PTR_(frame_time,                 DWORD)
PTR_(free_perk,                  char)
PTR_(game_global_vars,           long*)  // dynamic array of size == num_game_global_vars
PTR_(game_user_wants_to_quit,    DWORD)
PTR_(gcsd,                       fo::CombatGcsd*)
PTR_(gdBarterMod,                DWORD)
PTR_(gdNumOptions,               DWORD)
PTR_(gIsSteal,                   DWORD)
PTR_(glblmode,                   DWORD)
PTR_(gmouse_3d_current_mode,     long)
PTR_(gmouse_current_cursor,      long)
PTR_(gmovie_played_list,         BYTE) // array of 17 BYTE
PTR_(GoodColor,                  BYTE)
PTR_(GreenColor,                 BYTE)
PTR_(gsound_initialized,         DWORD)
PTR_(hit_location_penalty,       long) // array of 9 long
PTR_(holo_flag,                  DWORD)
PTR_(holopages,                  DWORD)
PTR_(hot_line_count,             DWORD)
PTR_(i_fid,                      DWORD)
PTR_(i_lhand,                    fo::GameObject*)
PTR_(i_rhand,                    fo::GameObject*)
PTRC(i_wid,                      DWORD)
PTR_(i_worn,                     fo::GameObject*)
PTR_(idle_func,                  void*)
PTR_(In_WorldMap,                DWORD) // moving on WorldMap
PTR_(info_line,                  DWORD)
PTRC(interfaceWindow,            DWORD)
PTR_(intfaceEnabled,             DWORD)
PTR_(intotal,                    DWORD)
PTR_(inven_dude,                 fo::GameObject*)
PTR_(inven_pid,                  DWORD)
PTR_(inven_scroll_dn_bid,        DWORD)
PTR_(inven_scroll_up_bid,        DWORD)
PTR_(inventry_message_file,      fo::MessageList)
PTR_(itemButtonItems,            fo::ItemButtonItem) // array of 2 ItemButtonItem, 0 - left, 1 - right
PTR_(itemCurrentItem,            long)  // 0 - left, 1 - right
PTR_(kb_lock_flags,              DWORD)
PTR_(language,                   char) // array of 32 char
PTR_(last_buttons,               DWORD)
PTR_(last_button_winID,          DWORD)
PTR_(last_level,                 DWORD)
PTR_(Level_pc,                   DWORD)
PTR_(Lifegiver,                  DWORD)
PTR_(LightGreyColor,             BYTE)
PTR_(LightRedColor,              BYTE)
PTR_(lipsFID,                    DWORD)
PTR_(list_com,                   DWORD)
PTR_(list_total,                 DWORD)
PTR_(loadingGame,                DWORD)
PTR_(LSData,                     DWORD)
PTR_(lsgwin,                     DWORD)
PTR_(main_ctd,                   fo::ComputeAttackResult)
PTR_(main_death_voiceover_done,  DWORD)
PTR_(main_window,                DWORD)
PTR_(map_elevation,              DWORD)
PTR_(map_global_vars,            long*)  // array
PTR_(map_number,                 DWORD)
PTR_(master_db_handle,           fo::PathNode*)
PTR_(master_volume,              DWORD)
PTR_(max,                        DWORD)
PTR_(maxScriptNum,               long)
PTR_(Meet_Frank_Horrigan,        bool)
PTR_(mouse_buttons,              DWORD)
PTR_(mouse_hotx,                 DWORD)
PTR_(mouse_hoty,                 DWORD)
PTR_(mouse_is_hidden,            DWORD)
PTR_(mouse_x_,                   DWORD)
PTR_(mouse_y,                    DWORD)
PTR_(mouse_y_,                   DWORD)
PTR_(movie_list,                 const char*) // array of 17 char*
PTR_(Mutate_,                    DWORD)
PTR_(name_color,                 DWORD)
PTR_(name_font,                  DWORD)
PTR_(name_sort_list,             DWORD)
PTR_(NearWhiteColor,             BYTE)
PTR_(num_caps,                   DWORD)
PTR_(num_game_global_vars,       DWORD)
PTR_(num_map_global_vars,        DWORD)
PTR_(num_windows,                DWORD)
PTR_(obj_dude,                   fo::GameObject*)
PTR_(objectTable,                fo::ObjectTable*) // array of 40000 pointers
PTR_(objItemOutlineState,        DWORD)
PTR_(optionRect,                 DWORD)
PTR_(optionsButtonDown,          DWORD)
PTR_(optionsButtonDown1,         DWORD)
PTR_(optionsButtonDownKey,       DWORD)
PTR_(optionsButtonUp,            DWORD)
PTR_(optionsButtonUp1,           DWORD)
PTR_(optionsButtonUpKey,         DWORD)
PTRC(optnwin,                    DWORD)
PTR_(outlined_object,            fo::GameObject*)
PTR_(partyMemberAIOptions,       DWORD)
PTR_(partyMemberCount,           DWORD)
PTR_(partyMemberLevelUpInfoList, DWORD*)
PTR_(partyMemberList,            fo::ObjectListData*) // dynamic array
PTR_(partyMemberMaxCount,        DWORD)
PTR_(partyMemberPidList,         DWORD*) // dynamic array
PTR_(patches,                    char*)
PTR_(paths,                      fo::PathNode*)  // array
PTR_(pc_crit_succ_eff,           fo::CritInfo) // array: 9 body parts, 6 effects
PTR_(pc_kill_counts,             DWORD)
PTR_(pc_name,                    char) // array of 32 char
PTR_(pc_proto,                   fo::Proto)
PTR_(pc_trait,                   long) // 2 of them
PTR_(PeanutButter,               BYTE)
PTR_(perk_data,                  fo::PerkInfo) // array of size == PERK_count
PTR_(perkLevelDataList,          fo::PartyMemberPerkListData*) // dynamic array, limited to (PERK_Count * partyMemberMaxCount)
PTRC(pip_win,                    DWORD)
PTR_(pipboy_message_file,        fo::MessageList)
PTR_(pipmesg,                    DWORD)
PTR_(preload_list_index,         DWORD)
PTR_(procTableStrs,              const char*)  // table of procId (from define.h) => procName map
PTR_(proto_main_msg_file,        fo::MessageList)
PTR_(proto_msg_files,            fo::MessageList)  // array of 6 elements
PTR_(protoLists,                 fo::ProtoList) // array of 11 elements
PTR_(ptable,                     DWORD)
PTR_(pud,                        DWORD)
PTR_(queue,                      fo::Queue*)
PTR_(quick_done,                 DWORD)
PTR_(read_callback,              DWORD)
PTR_(rectList,                   fo::RectList*)
PTR_(RedColor,                   BYTE)
PTR_(retvals,                    fo::ElevatorExit) // array: 24 elevators, 4 exits each
PTR_(rotation,                   DWORD)
PTR_(sampleRate,                 DWORD)
PTR_(scr_size,                   fo::BoundRect)
PTR_(scriptListInfo,             fo::ScriptListInfoItem*)  // dynamic array
PTR_(skill_data,                 fo::SkillInfo) // array of size == SKILL_count
PTRC(skldxwin,                   DWORD)
PTR_(slot_cursor,                DWORD)
PTR_(sndfx_volume,               DWORD)
PTR_(sneak_working,              DWORD) // DWORD var
PTR_(sound_music_path1,          char*)
PTR_(sound_music_path2,          char*)
PTR_(speech_volume,              DWORD)
PTR_(square,                     DWORD)
PTR_(squares,                    DWORD*)
PTR_(stack,                      DWORD) // array of 10 DWORD
PTR_(stack_offset,               DWORD) // array of 10 DWORD
PTR_(stat_data,                  fo::StatInfo) // array of size == STAT_real_max_stat
PTR_(stat_flag,                  DWORD)
PTR_(subtitleList,               fo::SubTitleList*)
PTR_(sWindows,                   fo::sWindow) // array of 16 sWindow
PTR_(Tag_,                       DWORD)
PTR_(tag_skill,                  DWORD)
PTR_(target_curr_stack,          DWORD)
PTR_(target_pud,                 DWORD*)
PTR_(target_stack,               DWORD) // array of 10 DWORD
PTR_(target_stack_offset,        DWORD) // array of 10 DWORD
PTR_(target_str,                 DWORD)
PTR_(target_xpos,                DWORD)
PTR_(target_ypos,                DWORD)
PTR_(text_char_width,            DWORD)
PTR_(text_height,                DWORD)
PTR_(text_max,                   DWORD)
PTR_(text_mono_width,            DWORD)
PTR_(text_object_index,          DWORD)
PTR_(text_object_list,           fo::FloatText*) // array of 20 FloatText*
PTR_(text_spacing,               DWORD)
PTR_(text_to_buf,                DWORD)
PTR_(text_width,                 DWORD)
PTR_(tile,                       DWORD)
PTR_(title_color,                DWORD)
PTR_(title_font,                 DWORD)
PTR_(trait_data,                 fo::TraitInfo) // array of size == TRAIT_count
PTR_(use_language,               DWORD)
PTR_(view_page,                  DWORD)
PTR_(wd_obj,                     DWORD)
PTR_(window,                     fo::Window*) // array of 50 Window*
PTR_(window_index,               DWORD) // array of 50 DWORD
PTR_(WhiteColor,                 BYTE)
PTR_(wmAreaInfoList,             DWORD)
PTRC(wmBkWin,                    DWORD)
PTR_(wmBkWinBuf,                 BYTE*)
PTR_(wmLastRndTime,              DWORD)
PTR_(wmMsgFile,                  fo::MessageList)
PTR_(wmNumHorizontalTiles,       DWORD)
PTR_(wmWorldOffsetX,             long)
PTR_(wmWorldOffsetY,             long)
PTR_(world_xpos,                 DWORD)
PTR_(world_ypos,                 DWORD)
PTR_(WorldMapCurrArea,           DWORD)
PTR_(YellowColor,                BYTE)

#undef PTR_
#undef PTRC
