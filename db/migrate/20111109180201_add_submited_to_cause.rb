class AddSubmitedToCause < ActiveRecord::Migration
  def self.up
  	add_column :causes, :submited, :integer, :default => 1
  end

  def self.down
    remove_column :causes, :submited
  end
end
