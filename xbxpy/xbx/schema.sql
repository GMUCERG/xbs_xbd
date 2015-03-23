create table platform(
    name text primary key,
    clock_hz int,
    pagesize int
);

create table operation(
    name primary key
);

create table primitive(
    operation text,
    name text,

    -- Checksum from xb
    checksumsmall text,

    -- Unused, too big for microcontrollers
    checksumlarge text,
    foreign key(operation) references operation(name),
    primary key(operation, name)
);
-- 
-- create table implementation(
--     operation text,
--     primitive text,
--     name text,
--     foreign key(operation) references operation(name),
--     foreign key(primitive) references primitive(name),
--     primary key(operation, primitive, name)
-- )


-- Configs do not have to be identical between build session and run session, so
-- long as checksums match and are verified at runtime
create table config(
    -- Sha256 of json
    hash text primary key,
    dump text unique
);


create table build_session ( 
    timestamp date,
    host text,
    xbx_version text,
    parallel boolean,
    config text,
    foreign key(config) references config(hash)
);

create table compiler(
    build_session int,
    platform text,
    idx int,
    cc_version text,
    cxx_version text,
    cc_version_full text,
    cxx_version_full text,
    cc text,
    cxx text,
    foreign key(build_session) references build_session(rowid),
    foreign key(platform) references platform(name),
    primary key(build_session, platform, idx)
);

create table build (
    platform text,
    operation text, 
    primitive text, 
    implementation text, 
    impl_checksum text,
    compiler_idx int, 

    exe_path text,
    hex_path text,
    parallel_make boolean,

    text int,
    data int,
    bss int,

    timestamp date,
    hex_checksum text,
    build_session int,

    -- True if not rebuilt from scratch
    rebuilt boolean,

    -- Checksum result
    checksumsmall_result text, 
    test_ok boolean, 

    foreign key(platform,build_session,compiler_idx) references compiler(platform, build_session, idx),
    foreign key(build_session) references build_session(rowid)
);



create table run_session (
    config text,
    host text,
    xbx_version text,
    build_session int,
    foreign key(config) references config(hash),
    foreign key(build_session) references build_session(rowid)
);



-- 
create table run (
    measured_cycles int,
    reported_cycles int,
    time_ns int,
    stackUsage int,
    min_power int,
    max_power int,
    avg_power int,
    median_power int,
    power_data text,
    timestamp date,
    build int,
    run_session int,
    foreign key(build) references build(rowid),
    foreign key(run_session) references run_session(rowid)
);

create table hash_run(
    input_length,
    run int,
    foreign key(run) references run(rowid)
);
create table aead_run(
    auth_data_length,
    plaintext_length,
    run int,
    foreign key(run) references run(rowid)
);
