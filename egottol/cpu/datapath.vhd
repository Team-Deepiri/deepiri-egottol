-- datapath.vhd
-- CPU datapath.  Instantiates the ALU and register file; contains the PC
-- register, operand muxes, and write-back mux.
--
-- PC update rules:
--   BEQ taken  : PC <= PC + 1 + sign_extend(imm12)   when ALU zero flag set
--   BNE taken  : PC <= PC + 1 + sign_extend(imm12)   when ALU zero flag clear
--   JMP        : PC <= zero_extend(imm12[9:0])        (absolute)
--   default    : PC <= PC + 1
--
-- Reset is synchronous, active-high.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity datapath is
    port (
        clk        : in  std_logic;
        rst        : in  std_logic;
        -- Control signals (from control_unit)
        reg_write  : in  std_logic;
        mem_read   : in  std_logic;    -- unused here; passed through for completeness
        mem_write  : in  std_logic;    -- unused here
        branch     : in  std_logic;
        mem_to_reg : in  std_logic;
        alu_src    : in  std_logic;
        jump       : in  std_logic;
        alu_op     : in  std_logic_vector(3 downto 0);
        -- Register addresses and immediate (from control_unit)
        cu_rd      : in  std_logic_vector(4 downto 0);
        cu_rs1     : in  std_logic_vector(4 downto 0);
        cu_rs2     : in  std_logic_vector(4 downto 0);
        cu_imm     : in  std_logic_vector(15 downto 0);
        -- Raw opcode[4] to distinguish BEQ/BNE (bit 0 of opcode = 0 -> BEQ, 1 -> BNE)
        -- We need the full opcode[4:0] to tell BEQ from BNE.
        -- Rather than re-expose opcode, we take a single 'bne' qualifier flag.
        bne        : in  std_logic;    -- '1' when instruction is BNE
        -- Instruction ROM data
        i_data     : in  std_logic_vector(31 downto 0);   -- not used internally; placeholder
        -- Data memory interface
        mem_rdata  : in  std_logic_vector(15 downto 0);
        -- Outputs
        pc         : out std_logic_vector(9 downto 0);
        mem_addr   : out std_logic_vector(9 downto 0);
        mem_wdata  : out std_logic_vector(15 downto 0);
        alu_result : out std_logic_vector(15 downto 0)
    );
end entity datapath;

architecture rtl of datapath is

    -- Internal PC (10-bit to address 1024-word instruction memory)
    signal pc_reg  : unsigned(9 downto 0) := (others => '0');
    signal pc_next : unsigned(9 downto 0);

    -- Register file interface
    signal rf_rd1  : std_logic_vector(15 downto 0);
    signal rf_rd2  : std_logic_vector(15 downto 0);
    signal rf_wd   : std_logic_vector(15 downto 0);

    -- ALU interface
    signal alu_a    : std_logic_vector(15 downto 0);
    signal alu_b    : std_logic_vector(15 downto 0);
    signal alu_res  : std_logic_vector(15 downto 0);
    signal alu_zero : std_logic;
    signal alu_neg  : std_logic;
    signal alu_cry  : std_logic;
    signal alu_ovf  : std_logic;

    -- Branch / jump helpers
    signal branch_offset : unsigned(9 downto 0);
    signal take_branch   : std_logic;

begin

    -- Register file instantiation
    u_regfile : entity work.register_file
        port map (
            clk => clk,
            rst => rst,
            we  => reg_write,
            rs1 => cu_rs1,
            rs2 => cu_rs2,
            rd  => cu_rd,
            wd  => rf_wd,
            rd1 => rf_rd1,
            rd2 => rf_rd2
        );

    -- ALU instantiation
    u_alu : entity work.alu
        port map (
            a             => alu_a,
            b             => alu_b,
            op            => alu_op,
            result        => alu_res,
            flag_zero     => alu_zero,
            flag_neg      => alu_neg,
            flag_carry    => alu_cry,
            flag_overflow => alu_ovf
        );

    -- ALU input A is always rs1
    alu_a <= rf_rd1;

    -- ALU input B mux: register rs2 or sign-extended immediate
    alu_b <= cu_imm when alu_src = '1' else rf_rd2;

    -- Write-back mux: ALU result or data memory read
    rf_wd <= mem_rdata when mem_to_reg = '1' else alu_res;

    -- Branch condition
    --   BEQ (bne='0'): take branch when zero flag set   (a == b)
    --   BNE (bne='1'): take branch when zero flag clear (a /= b)
    take_branch <= branch and (alu_zero xnor bne);
    -- xnor truth: bne=0 -> take_branch = branch & alu_zero
    --             bne=1 -> take_branch = branch & (not alu_zero)

    -- Branch offset: sign-extend imm16 -> 10 bits
    -- imm12 is already sign-extended to 16 bits in cu_imm; we take the low 10
    branch_offset <= unsigned(cu_imm(9 downto 0));

    -- PC next logic
    pc_next <=
        branch_offset + pc_reg + 1         when take_branch = '1' else
        unsigned(cu_imm(9 downto 0))       when jump = '1'        else
        pc_reg + 1;

    -- PC register (synchronous reset)
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                pc_reg <= (others => '0');
            else
                pc_reg <= pc_next;
            end if;
        end if;
    end process;

    -- Outputs
    pc         <= std_logic_vector(pc_reg);
    alu_result <= alu_res;

    -- Data memory address comes from ALU result (effective address = rs1 + imm)
    mem_addr   <= alu_res(9 downto 0);

    -- Data to write to memory is rs2
    mem_wdata  <= rf_rd2;

end architecture rtl;
