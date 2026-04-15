-- cpu_top.vhd
-- Top-level integration: wires together control_unit, datapath, imem, dmem.
-- io_in is available to future I/O-mapped instructions; io_out reflects the
-- most-recent ALU result (the write-back value).

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity cpu_top is
    port (
        clk    : in  std_logic;
        rst    : in  std_logic;
        io_in  : in  std_logic_vector(15 downto 0);
        pc     : out std_logic_vector(9 downto 0);
        io_out : out std_logic_vector(15 downto 0)
    );
end entity cpu_top;

architecture rtl of cpu_top is

    -- PC / instruction fetch
    signal pc_int   : std_logic_vector(9 downto 0);
    signal instr    : std_logic_vector(31 downto 0);

    -- Control unit outputs
    signal cu_rd         : std_logic_vector(4 downto 0);
    signal cu_rs1        : std_logic_vector(4 downto 0);
    signal cu_rs2        : std_logic_vector(4 downto 0);
    signal cu_imm        : std_logic_vector(15 downto 0);
    signal cu_reg_write  : std_logic;
    signal cu_mem_read   : std_logic;
    signal cu_mem_write  : std_logic;
    signal cu_branch     : std_logic;
    signal cu_mem_to_reg : std_logic;
    signal cu_alu_src    : std_logic;
    signal cu_jump       : std_logic;
    signal cu_alu_op     : std_logic_vector(3 downto 0);

    -- BNE qualifier: opcode bit 0 distinguishes BEQ (01110) from BNE (01111)
    signal bne_flag : std_logic;

    -- Datapath outputs
    signal dp_mem_addr  : std_logic_vector(9 downto 0);
    signal dp_mem_wdata : std_logic_vector(15 downto 0);
    signal dp_alu_result: std_logic_vector(15 downto 0);

    -- Data memory output
    signal dmem_dout : std_logic_vector(15 downto 0);

begin

    -- Instruction memory (ROM)
    u_imem : entity work.imem
        port map (
            addr  => pc_int,
            instr => instr
        );

    -- Control unit: purely combinational decode of current instruction
    u_cu : entity work.control_unit
        port map (
            instr      => instr,
            rd         => cu_rd,
            rs1        => cu_rs1,
            rs2        => cu_rs2,
            imm        => cu_imm,
            reg_write  => cu_reg_write,
            mem_read   => cu_mem_read,
            mem_write  => cu_mem_write,
            branch     => cu_branch,
            mem_to_reg => cu_mem_to_reg,
            alu_src    => cu_alu_src,
            jump       => cu_jump,
            alu_op     => cu_alu_op
        );

    -- BNE vs BEQ: opcode[0] = instr[27]; BEQ=01110, BNE=01111
    bne_flag <= instr(27);

    -- Datapath
    u_dp : entity work.datapath
        port map (
            clk        => clk,
            rst        => rst,
            reg_write  => cu_reg_write,
            mem_read   => cu_mem_read,
            mem_write  => cu_mem_write,
            branch     => cu_branch,
            mem_to_reg => cu_mem_to_reg,
            alu_src    => cu_alu_src,
            jump       => cu_jump,
            alu_op     => cu_alu_op,
            cu_rd      => cu_rd,
            cu_rs1     => cu_rs1,
            cu_rs2     => cu_rs2,
            cu_imm     => cu_imm,
            bne        => bne_flag,
            i_data     => instr,
            mem_rdata  => dmem_dout,
            pc         => pc_int,
            mem_addr   => dp_mem_addr,
            mem_wdata  => dp_mem_wdata,
            alu_result => dp_alu_result
        );

    -- Data memory (RAM)
    u_dmem : entity work.dmem
        port map (
            clk  => clk,
            we   => cu_mem_write,
            addr => dp_mem_addr,
            din  => dp_mem_wdata,
            dout => dmem_dout
        );

    -- Top-level output connections
    pc     <= pc_int;
    io_out <= dp_alu_result;

end architecture rtl;
