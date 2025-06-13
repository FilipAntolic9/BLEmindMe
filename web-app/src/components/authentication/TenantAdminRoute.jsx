import { Navigate } from 'react-router-dom';
import { useAuth } from '../../contexts/AuthContext';

const TenantAdminRoute = ({ children }) => {
    const { user } = useAuth();
    return user && user.role === 'TENANT_ADMIN' ? children : <Navigate to="/home" />;
};

export default TenantAdminRoute;