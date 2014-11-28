class AddProtocolToCauses < ActiveRecord::Migration
  def self.up
    add_column :causes, :protocol, :string
  end

  def self.down
    remove_column :causes, :protocol
  end
end
