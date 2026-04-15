library ieee;
use ieee.std_logic_1164.all;

entity ram_sync is
    generic (
        ADDR_WIDTH : positive := 8;
        DATA_WIDTH : positive := 8
    );
    port (
        clk     : in  std_logic;
        we      : in  std_logic;
        addr    : in  std_logic_vector(ADDR_WIDTH-1 downto 0);
        data_in : in  std_logic_vector(DATA_WIDTH-1 downto 0);
        data_out : out std_logic_vector(DATA_WIDTH-1 downto 0)
    );
end entity ram_sync;

architecture behavioral of ram_sync is
    type ram_type is array (0 to 2**ADDR_WIDTH-1) of std_logic_vector(DATA_WIDTH-1 downto 0);
    signal ram : ram_type;
begin
    process(clk)
    begin
        if rising_edge(clk) then
            if we = '1' then
                ram(to_integer(unsigned(addr))) <= data_in;
            end if;
            data_out <= ram(to_integer(unsigned(addr)));
        end if;
    end process;
end architecture behavioral;
