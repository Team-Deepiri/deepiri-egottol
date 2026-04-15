-- alu.vhd
-- 16-bit ALU for the simple RISC CPU.
-- All operations are purely combinational.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity alu is
    port (
        a            : in  std_logic_vector(15 downto 0);
        b            : in  std_logic_vector(15 downto 0);
        op           : in  std_logic_vector(3 downto 0);
        result       : out std_logic_vector(15 downto 0);
        flag_zero    : out std_logic;
        flag_neg     : out std_logic;
        flag_carry   : out std_logic;
        flag_overflow: out std_logic
    );
end entity alu;

architecture rtl of alu is

    -- ALU op encoding (matches opcode[3:0])
    constant OP_NOP : std_logic_vector(3 downto 0) := "0000";
    constant OP_ADD : std_logic_vector(3 downto 0) := "0001";
    constant OP_SUB : std_logic_vector(3 downto 0) := "0010";
    constant OP_AND : std_logic_vector(3 downto 0) := "0011";
    constant OP_OR  : std_logic_vector(3 downto 0) := "0100";
    constant OP_XOR : std_logic_vector(3 downto 0) := "0101";
    constant OP_NOT : std_logic_vector(3 downto 0) := "0110";
    constant OP_SLT : std_logic_vector(3 downto 0) := "0111";
    constant OP_SLL : std_logic_vector(3 downto 0) := "1000";
    constant OP_SRL : std_logic_vector(3 downto 0) := "1001";
    constant OP_SRA : std_logic_vector(3 downto 0) := "1010";
    constant OP_MOV : std_logic_vector(3 downto 0) := "1011";

begin

    process(a, b, op)
        variable va      : unsigned(15 downto 0);
        variable vb      : unsigned(15 downto 0);
        variable va_s    : signed(15 downto 0);
        variable vb_s    : signed(15 downto 0);
        variable vadd    : unsigned(16 downto 0);  -- 17-bit for carry
        variable vsub    : unsigned(16 downto 0);  -- 17-bit for borrow
        variable vres    : unsigned(15 downto 0);
        variable vres_s  : signed(15 downto 0);
        variable v_carry : std_logic;
        variable v_ovf   : std_logic;
    begin
        va    := unsigned(a);
        vb    := unsigned(b);
        va_s  := signed(a);
        vb_s  := signed(b);
        vres  := (others => '0');
        v_carry := '0';
        v_ovf   := '0';

        case op is
            when OP_NOP =>
                vres := (others => '0');

            when OP_ADD =>
                vadd    := ('0' & va) + ('0' & vb);
                vres    := vadd(15 downto 0);
                v_carry := vadd(16);
                -- overflow: same-sign operands produce opposite-sign result
                if (a(15) = b(15)) and (vres(15) /= a(15)) then
                    v_ovf := '1';
                end if;

            when OP_SUB =>
                vsub    := ('0' & va) - ('0' & vb);
                vres    := vsub(15 downto 0);
                v_carry := vsub(16);           -- borrow flag
                -- overflow: different-sign operands; result sign differs from a
                if (a(15) /= b(15)) and (vres(15) /= a(15)) then
                    v_ovf := '1';
                end if;

            when OP_AND =>
                vres := va and vb;

            when OP_OR =>
                vres := va or vb;

            when OP_XOR =>
                vres := va xor vb;

            when OP_NOT =>
                vres := not va;

            when OP_SLT =>
                if va_s < vb_s then
                    vres := to_unsigned(1, 16);
                else
                    vres := (others => '0');
                end if;

            when OP_SLL =>
                -- shift b left by 1
                vres := shift_left(vb, 1);

            when OP_SRL =>
                -- logical right shift b by 1
                vres := shift_right(vb, 1);

            when OP_SRA =>
                -- arithmetic right shift b by 1 (sign-extended)
                vres_s := shift_right(vb_s, 1);
                vres   := unsigned(vres_s);

            when OP_MOV =>
                vres := vb;

            when others =>
                vres := (others => '0');
        end case;

        result        <= std_logic_vector(vres);
        flag_zero     <= '1' when vres = 0 else '0';
        flag_neg      <= vres(15);
        flag_carry    <= v_carry;
        flag_overflow <= v_ovf;
    end process;

end architecture rtl;
