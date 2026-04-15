library ieee;
use ieee.std_logic_1164.all;

entity rom is
    generic (
        ADDR_WIDTH : positive := 8;
        DATA_WIDTH : positive := 8;
        INIT_FILE  : string := ""
    );
    port (
        addr    : in  std_logic_vector(ADDR_WIDTH-1 downto 0);
        data_out : out std_logic_vector(DATA_WIDTH-1 downto 0)
    );
end entity rom;

architecture behavioral of rom is
    type rom_type is array (0 to 2**ADDR_WIDTH-1) of std_logic_vector(DATA_WIDTH-1 downto 0);
    
    constant rom_init : rom_type := (
        0 => x"00",
        1 => x"01",
        2 => x"02",
        3 => x"03",
        4 => x"04",
        5 => x"05",
        6 => x"06",
        7 => x"07",
        8 => x"08",
        9 => x"09",
        10 => x"0A",
        11 => x"0B",
        12 => x"0C",
        13 => x"0D",
        14 => x"0E",
        15 => x"0F",
        others => x"00"
    );
begin
    data_out <= rom_init(to_integer(unsigned(addr)));
end architecture behavioral;
