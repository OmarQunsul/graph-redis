require 'redis'

redis = Redis.new

describe 'basic commands' do
  before do
    redis.flushdb
  end

  it "should list the graph nodes correctly" do
    redis.gnode 'graph1', 'a'
    nodes = redis.gnodes 'graph1', '*'
    nodes.should eq ['a']
    nodes = redis.gnodes 'graph1', '*'
    nodes.should eq ['a']
  end

  it "should be able to filter graph nodes" do
    redis.gnode 'graph1', 'country:jordan', 'country:germany', 'user:omar', 'user:john'
    countries = redis.gnodes 'graph1', 'country:*'
    expect(countries).to eq(%w(country:jordan country:germany))
    countries = redis.gnodes 'graph1', 'user:*'
    expect(countries).to eq(%w(user:omar user:john))
  end
end
