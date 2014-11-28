class AlterarDefaultSubmitedCauses < ActiveRecord::Migration
  def self.up
  	change_column :causes, :submited, :integer, :default => 0
  end

  def self.down
  	change_column :causes, :submited, :integer, :default => 1
  end
end
