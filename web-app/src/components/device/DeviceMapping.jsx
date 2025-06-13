// src/components/device/DeviceMapping.jsx
import { useState, useEffect } from 'react';
import { useNotification } from '../../contexts/NotificationContext';
import { getAllTenantDevices, assignTagToEsp } from '../../services/DeviceService';
import Navbar from '../NavBar';

const DeviceMapping = () => {
    const [espDevices, setEspDevices] = useState([]);
    const [tagDevices, setTagDevices] = useState([]);
    const [selectedEsp, setSelectedEsp] = useState('');
    const [selectedTag, setSelectedTag] = useState('');
    const [page, setPage] = useState(0);
    const [hasMore, setHasMore] = useState(true);
    const [loading, setLoading] = useState(false);
    const pageSize = 10;

    const { showNotification } = useNotification();

    const loadDevices = async (currentPage) => {
        try {
            setLoading(true);
            const response = await getAllTenantDevices(pageSize, currentPage);

            const newEspDevices = response.data.filter(device =>
                device.deviceProfileId.id === "c91cef10-43df-11f0-a544-db21b46190ed"
            );
            const newTagDevices = response.data.filter(device =>
                device.deviceProfileId.id === "76042e60-4150-11f0-a544-db21b46190ed"
            );

            if (currentPage === 0) {
                setEspDevices(newEspDevices);
                setTagDevices(newTagDevices);
            } else {
                setEspDevices(prev => [...prev, ...newEspDevices]);
                setTagDevices(prev => [...prev, ...newTagDevices]);
            }

            setHasMore(response.data.length === pageSize);
        } catch (err) {
            showNotification('Greška prilikom dohvaćanja uređaja: ' + err.message, 'error');
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadDevices(0);
    }, []);

    const handleAssignment = async (e) => {
        e.preventDefault();
        if (!selectedEsp || !selectedTag) {
            showNotification('Molimo odaberite oba uređaja', 'error');
            return;
        }

        try {
            await assignTagToEsp(selectedEsp, selectedTag);
            showNotification('Uređaji uspješno povezani!', 'success');
            setSelectedEsp('');
            setSelectedTag('');
        } catch (err) {
            showNotification('Greška prilikom povezivanja uređaja: ' + err.message, 'error');
        }
    };

    return (
        <div className="min-h-screen bg-gray-100">
            <Navbar />
            <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
                <h2 className="text-2xl font-bold mb-6 text-center">Povezivanje uređaja</h2>

                <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                    {/* ESP32 Devices Table */}
                    <div className="bg-white p-6 rounded-lg shadow">
                        <h3 className="text-xl font-semibold mb-4">Uređaji za provjeru (ESP32)</h3>
                        <div className="overflow-x-auto">
                            <table className="min-w-full divide-y divide-gray-200">
                                <thead className="bg-gray-50">
                                <tr>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Odaberi</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Naziv</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Opis</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">MAC Adresa</th>
                                </tr>
                                </thead>
                                <tbody className="bg-white divide-y divide-gray-200">
                                {espDevices.map(device => (
                                    <tr key={device.id.id}>
                                        <td className="px-6 py-4">
                                            <input
                                                type="radio"
                                                name="espDevice"
                                                value={device.id.id}
                                                checked={selectedEsp === device.id.id}
                                                onChange={(e) => setSelectedEsp(e.target.value)}
                                            />
                                        </td>
                                        <td className="px-6 py-4 whitespace-nowrap">{device.name}</td>
                                        <td className="px-6 py-4 whitespace-nowrap">{device.type}</td>
                                        <td className="px-6 py-4 whitespace-nowrap">{device.label}</td>
                                    </tr>
                                ))}
                                </tbody>
                            </table>
                        </div>
                    </div>

                    {/* BLE Tags Table */}
                    <div className="bg-white p-6 rounded-lg shadow">
                        <h3 className="text-xl font-semibold mb-4">Stvari za praćenje (BLE Tags)</h3>
                        <div className="overflow-x-auto">
                            <table className="min-w-full divide-y divide-gray-200">
                                <thead className="bg-gray-50">
                                <tr>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Odaberi</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Naziv</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Opis</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">MAC Adresa</th>
                                </tr>
                                </thead>
                                <tbody className="bg-white divide-y divide-gray-200">
                                {tagDevices.map(device => (
                                    <tr key={device.id.id}>
                                        <td className="px-6 py-4">
                                            <input
                                                type="radio"
                                                name="tagDevice"
                                                value={device.id.id}
                                                checked={selectedTag === device.id.id}
                                                onChange={(e) => setSelectedTag(e.target.value)}
                                            />
                                        </td>
                                        <td className="px-6 py-4 whitespace-nowrap">{device.name}</td>
                                        <td className="px-6 py-4 whitespace-nowrap">{device.type}</td>
                                        <td className="px-6 py-4 whitespace-nowrap">{device.label}</td>
                                    </tr>
                                ))}
                                </tbody>
                            </table>
                        </div>
                    </div>
                </div>

                {/* Submit Button */}
                <div className="mt-6 flex justify-center">
                    <button
                        onClick={handleAssignment}
                        className="bg-blue-600 text-white py-2 px-8 rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2"
                        disabled={!selectedEsp || !selectedTag}
                    >
                        Poveži uređaje
                    </button>
                </div>

                {hasMore && (
                    <div className="mt-4 text-center">
                        <button
                            onClick={() => {
                                setPage(prev => prev + 1);
                                loadDevices(page + 1);
                            }}
                            disabled={loading}
                            className="bg-gray-200 text-gray-700 px-4 py-2 rounded-md hover:bg-gray-300 disabled:bg-gray-100"
                        >
                            {loading ? 'Učitavam...' : 'Učitaj više'}
                        </button>
                    </div>
                )}
            </div>
        </div>
    );
};

export default DeviceMapping;